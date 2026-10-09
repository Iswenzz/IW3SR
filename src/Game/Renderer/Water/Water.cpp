#include "Water.hpp"

#include "Game/Renderer/Drawing/Text.hpp"
#include "Game/Renderer/Materials.hpp"
#include "Game/Renderer/Portal/Portal.hpp"
#include "Game/System/Dvar.hpp"

namespace IW3SR
{
	// What IzFF's converter names its water technique set, the image the shader reads a planar
	// reflection from, and the constant that says the reflection is live and at what height.
	constexpr std::string_view WATER_TECHNIQUE_SET = "iz_flow";
	constexpr std::string_view WATER_PLANAR_IMAGE = "iz_water_planar";
	constexpr std::string_view WATER_PLANAR_CONSTANT = "waterPlanar";

	// Surfaces this close in height are one sheet of water.
	constexpr float PLANE_MERGE = 2.0f;

	// The clip plane sits this far above the water, so its own surface never draws into its reflection.
	constexpr float PLANE_LIFT = 1.0f;

	// Frames the material count has to hold still before a level is taken to have no water.
	constexpr int DiscoverSettleScans = 60;

	// GfxViewInfo's real stride, the one GPortal::BeginCommandList inlines.
	constexpr size_t VIEW_INFO_STRIDE = 0x67B0;

	static bool IsWaterTechniqueSet(const MaterialTechniqueSet* techniques)
	{
		if (!techniques || !techniques->name)
			return false;

		return std::string_view(techniques->name).find(WATER_TECHNIQUE_SET) != std::string_view::npos;
	}

	void GWater::Initialize()
	{
		Enabled = Dvar::RegisterBool("sr_water_reflection", DVAR_SAVED, "Mirror the scene in IzFF's water", true);
		Scale = Dvar::RegisterFloat("sr_water_scale", DVAR_SAVED, "Water reflection resolution, relative to the screen",
			0.5f, 0.125f, 1.0f);
		Distance = Dvar::RegisterFloat("sr_water_distance", DVAR_SAVED,
			"Stop drawing water reflections past this distance", 6000.0f, 0.0f, 50000.0f);
		Debug = Dvar::RegisterBool("sr_water_debug", DVAR_TEMP, "Show what the water reflection pass is finding", false);
		Threaded = Dvar::Find("r_smp_backend");
	}

	void GWater::Shutdown()
	{
		Restore();
		Surfaces.clear();
		Planes.clear();
		KnownWorld = nullptr;
		Target = nullptr;
		TargetSize = {};
	}

	// Run from the R_BeginFrame hook before the portal pass, the same way: the reflection is a whole
	// frame of its own, drawn into a corner of the frame buffer and copied out before the real frame
	// paints over it (GPortal::Render says why each step is there).
	void GWater::BeginFrame()
	{
		if (GPortal::Rendering)
			return;

		const bool ready = Ready();
		if (ready && !Surfaces.empty() && Threaded && Threaded->current.enabled)
			R_SyncRenderThread();
		Restore();
		if (!ready)
			return void(DebugStage = "not ready");

		Discover();
		if (Surfaces.empty())
			return void(DebugStage = "no iz_flow material with iz_water_planar and waterPlanar");
		if (!Enabled || !Enabled->current.enabled)
			return void(DebugStage = "disabled");

		const WaterPlane* plane = Pick();
		if (!plane)
			return void(DebugStage = "no plane in view below the camera");
		DebugHeight = plane->Height;

		const bool threaded = Threaded && Threaded->current.enabled;
		if (threaded)
			Threaded->current.enabled = false;
		const bool drawn = Render(*plane);
		if (threaded)
			Threaded->current.enabled = true;

		DebugStage = drawn ? "mirrored" : "render or capture failed";
		if (drawn)
		{
			DebugRendered++;
			Bind(plane->Height);
		}
	}

	void GWater::DrawDebug()
	{
		if (!Debug || !Debug->current.enabled)
			return;

		static GText text{ "", FONT_NORMAL, 10, 200, 1.2f, vec4(0, 1, 1, 1) };
		text.Value = std::format("water: {}\nmaterials {}  planes {}  height {:.1f}  rendered {}", DebugStage,
			Surfaces.size(), Planes.size(), DebugHeight, DebugRendered);
		text.Render();
	}

	// The world has been drawn, so the image and the constants go back to what the zone holds.
	void GWater::EndFrame()
	{
		Restore();
	}

	bool GWater::Ready()
	{
		if (!dx || !dx->device || dx->deviceLost || dx->inScene)
			return false;
		if (!rgp || !rgp->world)
			return false;
		if (!cgs || cgs->isLoading)
			return false;
		return client_ui && client_ui->connectionState == CA_ACTIVE;
	}

	// Every material drawing through iz_flow, found by technique set as the portal surfaces are, and
	// rescanned while none turn up because the material list is still being sorted in on a level's
	// first frames.
	void GWater::Discover()
	{
		const int count = std::min(rgp->materialCount, GMaterials::PoolSize());

		if (KnownWorld != rgp->world)
		{
			Restore();
			Surfaces.clear();
			Planes.clear();
			KnownWorld = rgp->world;
			KnownCount = -1;
			SettledScans = 0;
		}
		else if (!Surfaces.empty() || SettledScans >= DiscoverSettleScans)
			return;

		SettledScans = count == KnownCount ? SettledScans + 1 : 0;
		KnownCount = count;

		Material** const sorted = GMaterials::Sorted();
		for (int i = 0; i < count; i++)
		{
			Material* material = sorted[i];
			if (!material || !material->textureTable || !IsWaterTechniqueSet(material->techniqueSet))
				continue;

			WaterSurface surface{ material };
			for (int t = 0; t < material->textureCount; t++)
			{
				GfxImage* image = material->textureTable[t].u.image;
				if (image && image->name && WATER_PLANAR_IMAGE == image->name)
					surface.Image = image;
			}
			for (int c = 0; material->constantTable && c < material->constantCount; c++)
			{
				MaterialConstantDef& constant = material->constantTable[c];
				const std::string_view name(constant.name, strnlen(constant.name, sizeof(constant.name)));
				if (name == WATER_PLANAR_CONSTANT)
					surface.Plane = &constant.literal;
			}
			if (surface.Image && surface.Plane)
				Surfaces.push_back(surface);
		}
		if (!Surfaces.empty())
			FindPlanes();
	}

	// The world's water surfaces grouped by height. A converted map's water is flat, so each surface's
	// bounds are one height.
	void GWater::FindPlanes()
	{
		Planes.clear();

		const GfxWorld* world = rgp->world;
		if (!world->dpvs.surfaces)
			return;

		for (int i = 0; i < world->surfaceCount; i++)
		{
			const GfxSurface& surface = world->dpvs.surfaces[i];
			const bool water = std::ranges::any_of(Surfaces,
				[&](const WaterSurface& known) { return known.Material == surface.material; });
			if (!water)
				continue;

			const vec3 mins(surface.bounds[0][0], surface.bounds[0][1], surface.bounds[0][2]);
			const vec3 maxs(surface.bounds[1][0], surface.bounds[1][1], surface.bounds[1][2]);
			const float height = (mins.z + maxs.z) * 0.5f;

			const auto plane = std::ranges::find_if(Planes,
				[&](const WaterPlane& known) { return std::abs(known.Height - height) < PLANE_MERGE; });
			if (plane == Planes.end())
				Planes.push_back({ height, mins, maxs });
			else
			{
				plane->Mins = glm::min(plane->Mins, mins);
				plane->Maxs = glm::max(plane->Maxs, maxs);
			}
		}
	}

	// The nearest sheet the camera is above and can see. Only one is mirrored a frame; the others read
	// the sky, since the shader matches the reflection to its own height.
	const WaterPlane* GWater::Pick()
	{
		const refdef_s& view = cgs->refdef;
		const float limit = Distance->current.value;
		const float tanX = view.tanHalfFovX;
		const float tanY = view.tanHalfFovY;

		const WaterPlane* best = nullptr;
		float nearest = std::numeric_limits<float>::max();
		for (const WaterPlane& plane : Planes)
		{
			if (view.vieworg.z <= plane.Height + PLANE_LIFT)
				continue;

			const float distance = glm::length(glm::clamp(view.vieworg, plane.Mins, plane.Maxs) - view.vieworg);
			if (limit > 0.0f && distance > limit)
				continue;

			// Its bounding sphere against the frustum, as GPortal::Visible tests a portal.
			const vec3 offset = (plane.Mins + plane.Maxs) * 0.5f - view.vieworg;
			const float radius = glm::length(plane.Maxs - plane.Mins) * 0.5f;
			const float z = glm::dot(offset, view.viewaxis[0]);
			const float x = std::abs(glm::dot(offset, view.viewaxis[1]));
			const float y = std::abs(glm::dot(offset, view.viewaxis[2]));
			if (z < -radius || x - z * tanX > radius * std::sqrt(1.0f + tanX * tanX)
				|| y - z * tanY > radius * std::sqrt(1.0f + tanY * tanY))
				continue;

			if (distance < nearest)
			{
				nearest = distance;
				best = &plane;
			}
		}
		return best;
	}

	bool GWater::Render(const WaterPlane& plane)
	{
		const auto& frame = gfx_renderTargets[R_RENDERTARGET_FRAME_BUFFER];
		const float scale = std::clamp(Scale->current.value, 0.125f, 1.0f);
		const vec2 size = glm::floor(vec2(frame.width, frame.height) * scale);

		if (size.x < 1.0f || size.y < 1.0f || !Resize(size))
			return false;

		// The camera mirrored through the water. Mirroring every axis would leave a left-handed frame,
		// so its up axis is turned back over: the picture comes out upside down, and the shader reads
		// it upside down. Fov and aspect stay the main view's, so it lines up by screen position.
		static refdef_s view;
		view = cgs->refdef;
		view.x = 0;
		view.y = 0;
		view.width = static_cast<uint32_t>(size.x);
		view.height = static_cast<uint32_t>(size.y);
		view.vieworg.z = 2.0f * plane.Height - view.vieworg.z;
		for (int i = 0; i < 3; i++)
			view.viewaxis[i].z = -view.viewaxis[i].z;
		view.viewaxis[2] = -view.viewaxis[2];

		R_SyncRenderThread();
		GPortal::Rendering = true;
		R_BeginFrame_h();
		GPortal::BeginCommandList();
		R_ClearScene(0);
		R_SetLodOrigin(&view);
		R_RenderScene(&view);

		// The camera sits under the ground, so everything below the water is clipped away, the lake bed
		// included. D3D9 takes the plane in clip space when shaders draw, which is the world plane
		// through the inverse of the view-projection the engine just built for this view.
		bool clipped = false;
		const GfxBackEndData* data = gfx_frontEndDataOut ? *gfx_frontEndDataOut : nullptr;
		if (data && data->viewInfo && data->viewInfoCount > 0)
		{
			const auto* info = reinterpret_cast<const GfxViewInfo*>(
				reinterpret_cast<const uint8_t*>(data->viewInfo) + (data->viewInfoCount - 1) * VIEW_INFO_STRIDE);
			const GfxMatrix& inverse = info->viewParms.inverseViewProjectionMatrix;
			const float world[4] = { 0.0f, 0.0f, 1.0f, -(plane.Height + PLANE_LIFT) };

			float clip[4];
			for (int r = 0; r < 4; r++)
				clip[r] = inverse.m[r][0] * world[0] + inverse.m[r][1] * world[1] + inverse.m[r][2] * world[2]
					+ inverse.m[r][3] * world[3];

			clipped = SUCCEEDED(dx->device->SetClipPlane(0, clip))
				&& SUCCEEDED(dx->device->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0));
		}

		R_EndFrame();
		R_IssueRenderCommands(1);
		if (clipped)
			dx->device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
		GPortal::Rendering = false;

		return clipped && Capture();
	}

	bool GWater::Resize(const vec2& size)
	{
		if (Target && TargetSize == size)
			return true;

		Restore();

		TextureSpecification spec;
		spec.ID = "water_reflection";
		spec.Size = size;
		spec.Level = 1;
		spec.Usage = TextureUsage::RenderTarget;
		spec.Pool = TexturePool::Default;

		AssetManager::Remove(spec.ID);
		Target = Texture::Create(spec);
		TargetSize = size;

		// A failed create falls back to the default texture, which is not this size.
		if (!Target || Target->GetSize() != size)
		{
			Target = nullptr;
			return false;
		}
		return true;
	}

	// The corner the pass drew into, copied into the whole target (GPortal::Capture).
	bool GWater::Capture()
	{
		const auto& frame = gfx_renderTargets[R_RENDERTARGET_FRAME_BUFFER];
		const auto texture = std::static_pointer_cast<DX9Texture>(Target);

		if (!frame.surface.color || !texture || !texture->Surface)
			return false;

		uint32_t bound = GPortal::Unbind(texture->Data);

		const RECT source = { 0, 0, static_cast<LONG>(TargetSize.x), static_cast<LONG>(TargetSize.y) };
		const bool copied =
			SUCCEEDED(dx->device->StretchRect(frame.surface.color, &source, texture->Surface, nullptr, D3DTEXF_LINEAR));

		for (uint32_t slot = 0; bound; slot++, bound >>= 1)
		{
			if (bound & 1)
				dx->device->SetTexture(slot, texture->Data);
		}
		return copied;
	}

	// Every water material's planar image swapped for the reflection, and its constant told the height
	// the reflection was drawn for. Materials share the image, so all the originals are taken first.
	void GWater::Bind(float height)
	{
		const auto texture = std::static_pointer_cast<DX9Texture>(Target);
		if (!texture || !texture->Data || Swapped)
			return;

		for (WaterSurface& surface : Surfaces)
			surface.Original = surface.Image->texture.map;
		for (WaterSurface& surface : Surfaces)
		{
			surface.Image->texture.map = texture->Data;
			*surface.Plane = vec4(height, 1.0f, 0.0f, 0.0f);
		}
		Swapped = true;
	}

	// Handed back before the frame ends: the engine frees the image's texture between frames, and one
	// of ours left in it would be released twice (GPortal::EndFrame).
	void GWater::Restore()
	{
		if (!Swapped)
			return;

		Swapped = false;
		for (WaterSurface& surface : Surfaces)
		{
			*surface.Plane = vec4(0.0f);
			if (surface.Original)
				surface.Image->texture.map = surface.Original;
		}
	}
}
