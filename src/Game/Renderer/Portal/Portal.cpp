#include "Portal.hpp"

#include "Game/Renderer/Drawing/Text.hpp"
#include "Game/System/Dvar.hpp"

namespace IW3SR
{
	constexpr std::string_view PORTAL_MODEL_PREFIX = "portal_dummy_";
	constexpr std::string_view PORTAL_TECHNIQUE_SET = "portal_view";

	// The linker binds a vertex-format variant of the technique set per material: xmodels get
	// "mc_portal_view", world surfaces "wc_portal_view". Match the stem rather than the asset name.
	static bool IsPortalTechniqueSet(const MaterialTechniqueSet* techniques)
	{
		if (!techniques || !techniques->name)
			return false;

		return std::string_view(techniques->name).find(PORTAL_TECHNIQUE_SET) != std::string_view::npos;
	}

	void GPortal::Initialize()
	{
		Enabled = Dvar::RegisterBool("sr_portal_view", DVAR_SAVED, "Render the far side of a portal into its surface",
			true);
		Scale = Dvar::RegisterFloat("sr_portal_scale", DVAR_SAVED, "Portal view resolution, relative to the screen",
			0.5f, 0.125f, 1.0f);
		Distance = Dvar::RegisterFloat("sr_portal_distance", DVAR_SAVED,
			"Stop rendering portal views past this distance", 3000.0f, 0.0f, 20000.0f);
		Debug = Dvar::RegisterBool("sr_portal_debug", DVAR_TEMP, "Show what the portal view pass is finding", false);
		Threaded = Dvar::Find("r_smp_backend");
	}

	void GPortal::Shutdown()
	{
		Restore();
		Surfaces.clear();
		KnownWorld = nullptr;

		for (auto& target : Targets)
		{
			target.Color = nullptr;
			target.Size = {};
		}
	}

	// Hooked onto R_BeginFrame rather than SCR_UpdateFrame: CoD4x replaces the latter with a jmp of
	// its own, so whichever of us patches last wins. R_BeginFrame it only ever calls.
	//
	// Everything happens before the engine opens its frame. Each portal view is a complete standalone
	// frame -- the sequence R_GenerateReflectionRawData uses to bake probes -- drawn into a corner of
	// the frame buffer and copied out before the real frame paints over it.
	void GPortal::BeginFrame()
	{
		if (Rendering)
		{
			R_BeginFrame_h();
			return;
		}
		DebugFrames++;

		if (Ready())
		{
			Discover();

			PortalEndpoint pair[2];
			if (Collect(pair))
			{
				Missed = 0;
				Blank();

				// R_HandOffToBackend queues the pass on the render thread when r_smp_backend is set,
				// so R_IssueRenderCommands returns before anything has drawn and the copy picks up
				// whatever the frame buffer still held -- the last presented frame, HUD included.
				const bool threaded = Threaded && Threaded->current.enabled;
				if (threaded)
					Threaded->current.enabled = false;

				Render(0, pair[0], pair[1]);
				Render(1, pair[1], pair[0]);

				if (threaded)
					Threaded->current.enabled = true;

				Bind(Targets[0], pair[0]);
				Bind(Targets[1], pair[1]);
			}
			// A quad can drop out of the entity list for a frame or two while the snapshot updates.
			// Blanking on the first miss makes that flicker black, so let the last view stand briefly.
			else if (++Missed > 5)
			{
				Blank();
			}
		}
		R_BeginFrame_h();
	}

	bool GPortal::Ready()
	{
		DebugStage = "ok";

		if (!Enabled || !Enabled->current.enabled)
			return DebugStage = "disabled", false;
		if (!dx || !dx->device || dx->deviceLost || dx->inScene)
			return DebugStage = "device busy", false;
		if (!rgp || !rgp->world)
			return DebugStage = "no world", false;
		if (!cgs || cgs->isLoading)
			return DebugStage = "loading", false;
		if (!client_ui || client_ui->connectionState != CA_ACTIVE)
			return DebugStage = "not connected", false;
		return true;
	}

	// Every material drawing through portal_view is a portal surface. Finding them by technique set
	// rather than by name means the pass stays off entirely on a fastfile still built on unlit_blend.
	// Rescans while nothing has been found: the material list is sorted by R_BeginFrame, which runs
	// after this, so on the first frames of a level it can still be empty or incomplete.
	void GPortal::Discover()
	{
		if (KnownWorld == rgp->world && !Surfaces.empty())
			return;

		Restore();
		Surfaces.clear();
		KnownWorld = rgp->world;

		for (int i = 0; i < rgp->materialCount; i++)
		{
			Material* material = rgp->sortedMaterials[i];
			if (!material || !material->textureCount || !material->textureTable)
				continue;

			if (!IsPortalTechniqueSet(material->techniqueSet))
				continue;

			GfxImage* image = material->textureTable[0].u.image;
			if (!image)
				continue;

			Surfaces.push_back({ material, image, image->texture.map });
		}
	}

	// The mod spawns a flat quad ("portal_dummy_<colour>") on the surface, oriented so that its forward
	// axis is the surface normal. Two of them, of different colours, make a linked pair.
	// Several players' portals are in the snapshot at once -- the mod only hides other people's
	// client side -- so take the nearest quad of each colour rather than the first two found.
	bool GPortal::Collect(PortalEndpoint (&pair)[2])
	{
		const snapshot_s* snap = cgs->snap;
		if (!snap)
			return false;

		const vec3 eye = cgs->refdef.vieworg;
		std::string_view colors[2];
		float nearest[2] = { FLT_MAX, FLT_MAX };
		int found = 0;

		DebugModels = 0;
		DebugMatched = 0;

		// Walk the snapshot rather than all of pEntityLastXModel: that table keeps its entry until the
		// client frees the entity's DObj, so a deleted portal lingers there with its last position and
		// the pass carries on rendering a portal that is no longer in the world.
		for (int n = 0; n < snap->numEntities; n++)
		{
			const int i = snap->entities[n].number;
			if (i < 0 || i >= MAX_GENTITIES)
				continue;

			const XModel* model = cgs->pEntityLastXModel[i];
			if (!model || !model->name || !model->materialHandles || !model->numsurfs)
				continue;

			const std::string_view name = model->name;
			if (!name.starts_with(PORTAL_MODEL_PREFIX))
				continue;

			DebugModels++;

			Material* surface = model->materialHandles[0];
			const MaterialTechniqueSet* techniques = surface ? surface->techniqueSet : nullptr;

			DebugMaterial = surface && surface->info.name ? surface->info.name : "null";
			DebugTechniques = techniques && techniques->name ? techniques->name : "null";

			if (!IsPortalTechniqueSet(techniques))
				continue;

			const std::string_view color = name.substr(PORTAL_MODEL_PREFIX.size());
			const centity_s& entity = cg_entities[i];
			const float range = glm::distance(eye, entity.pose.origin);

			int slot = -1;
			if (found && colors[0] == color)
				slot = 0;
			else if (found > 1 && colors[1] == color)
				slot = 1;
			else if (found < 2)
				slot = found++;

			if (slot < 0 || range >= nearest[slot])
				continue;

			PortalEndpoint& endpoint = pair[slot];
			endpoint.Origin = entity.pose.origin;
			Math::AngleVectors(entity.pose.angles, endpoint.Forward, endpoint.Right, endpoint.Up);
			endpoint.Surface = surface;
			endpoint.Entity = i;

			colors[slot] = color;
			nearest[slot] = range;
		}
		DebugMatched = found;

		if (found < 2)
			return false;

		const float distance = Distance->current.value;
		return distance <= 0.0f || nearest[0] < distance || nearest[1] < distance;
	}

	// R_BeginSharedCmdList and R_ClearClientCmdList2D are inlined into their callers, so there is no
	// address to call. Both are a single store, reproduced here. Without them the backend dispatches
	// a command list that was never pointed anywhere and jumps through a garbage handler.
	void GPortal::BeginCommandList()
	{
		// The declared GfxViewInfo is not the size the engine actually uses, so its stride and the
		// offset of its command pointer are taken from the code that inlined these two stores.
		constexpr size_t VIEW_INFO_STRIDE = 0x67B0;
		constexpr size_t VIEW_INFO_CMDS = 0x5688;

		static_assert(offsetof(GfxBackEndData, viewInfoCount) == 0x11E6C4, "GfxBackEndData layout moved");
		static_assert(offsetof(GfxBackEndData, viewInfo) == 0x11E6C8, "GfxBackEndData layout moved");
		static_assert(offsetof(GfxBackEndData, cmds) == 0x11E6CC, "GfxBackEndData layout moved");

		const GfxCmdArray* commands = gfx_cmdList ? *gfx_cmdList : nullptr;
		if (!commands || !gfx_frontEndDataOut || !gfx_frontEndDataOut->viewInfo)
			return;

		gfx_frontEndDataOut->cmds = &commands->cmds[commands->usedTotal];

		auto* view = reinterpret_cast<uint8_t*>(gfx_frontEndDataOut->viewInfo)
			+ gfx_frontEndDataOut->viewInfoCount * VIEW_INFO_STRIDE;
		*reinterpret_cast<const void**>(view + VIEW_INFO_CMDS) = nullptr;
	}

	// Rotate a vector 180 degrees about the exit portal's up axis -- the standard portal transform.
	// The flip puts the camera behind the exit plane, so looking forward emerges out of the wall.
	vec3 GPortal::Through(const PortalEndpoint& into, const PortalEndpoint& out, const vec3& v, bool position)
	{
		const vec3 d = position ? v - into.Origin : v;
		const vec3 local = { glm::dot(d, into.Forward), glm::dot(d, into.Right), glm::dot(d, into.Up) };
		const vec3 result = out.Forward * -local.x + out.Right * -local.y + out.Up * local.z;

		return position ? out.Origin + result : result;
	}

	void GPortal::Render(int index, const PortalEndpoint& into, const PortalEndpoint& out)
	{
		const auto& frame = gfx_renderTargets[R_RENDERTARGET_FRAME_BUFFER];
		const float scale = std::clamp(Scale->current.value, 0.125f, 1.0f);
		const vec2 size = glm::floor(vec2(frame.width, frame.height) * scale);

		if (size.x < 1.0f || size.y < 1.0f)
			return;

		PortalTarget& target = Targets[index];
		if (!Resize(index, target, size))
			return;

		// Everything but the camera is inherited, so fov, vision set, primary lights and scene time
		// stay identical to the main view.
		//
		// The pass covers the whole frame buffer rather than a corner of it. A smaller viewport left
		// the rest of the captured region holding whatever the last frame drew there, which showed up
		// as a hard seam with the wall behind the portal on one side of it. The saving is not worth
		// depending on exactly which stage of the backend honours refdef's viewport.
		static refdef_s view;
		view = cgs->refdef;
		view.x = 0;
		view.y = 0;
		view.width = frame.width;
		view.height = frame.height;
		view.vieworg = Through(into, out, cgs->refdef.vieworg, true);

		for (int i = 0; i < 3; i++)
			view.viewaxis[i] = Through(into, out, cgs->refdef.viewaxis[i], false);

		Rendering = true;
		R_SyncRenderThread();
		R_BeginFrame_h();
		BeginCommandList();
		R_ClearScene(0);
		R_SetLodOrigin(&view);
		R_RenderScene(&view);
		R_EndFrame();
		R_IssueRenderCommands(1); // render without presenting
		R_SyncRenderThread();     // r_smp_backend hands the pass off, so wait before copying
		Rendering = false;

		if (Capture(target))
			DebugCaptured++;
	}

	bool GPortal::Resize(int index, PortalTarget& target, const vec2& size)
	{
		if (target.Color && target.Size == size)
			return true;

		Blank();

		TextureSpecification spec;
		spec.ID = std::format("portal_view_{}", index);
		spec.Size = size;
		spec.Level = 1;
		spec.Usage = TextureUsage::RenderTarget;
		spec.Pool = TexturePool::Default;

		AssetManager::Remove(spec.ID);
		target.Color = Texture::Create(spec);
		target.Size = size;

		// Texture::Create falls back to the default texture on failure, which is neither the right
		// size nor a render target -- take the size as proof the real one came back.
		if (!target.Color || target.Color->GetSize() != size)
		{
			target.Color = nullptr;
			return false;
		}
		return true;
	}

	bool GPortal::Capture(PortalTarget& target)
	{
		const auto& frame = gfx_renderTargets[R_RENDERTARGET_FRAME_BUFFER];
		const auto texture = std::static_pointer_cast<DX9Texture>(target.Color);

		if (!frame.surface.color || !texture || !texture->Surface)
			return false;

		// The pass we just drew may have left this texture on a sampler, and D3D9 will not blit into
		// a bound one. Detach it for the copy and put it straight back, so the engine's own record of
		// what is bound where stays true.
		uint32_t bound = Unbind(texture->Data);

		// Whole frame buffer into the whole target: the pass renders full screen and the shader reads
		// this by screen position, so the two have to describe the same rectangle. sr_portal_scale
		// only decides how much resolution is kept.
		const RECT source = { 0, 0, static_cast<LONG>(frame.width), static_cast<LONG>(frame.height) };
		const bool copied = SUCCEEDED(dx->device->StretchRect(frame.surface.color, &source, texture->Surface,
			nullptr, D3DTEXF_LINEAR));

		for (uint32_t slot = 0; bound; slot++, bound >>= 1)
		{
			if (bound & 1)
				dx->device->SetTexture(slot, texture->Data);
		}
		return copied;
	}

	// Clears every sampler holding this texture and returns them as a bitmask.
	uint32_t GPortal::Unbind(IDirect3DTexture9* texture)
	{
		constexpr uint32_t SAMPLER_COUNT = 8;
		uint32_t bound = 0;

		for (uint32_t slot = 0; slot < SAMPLER_COUNT; slot++)
		{
			IDirect3DBaseTexture9* current = nullptr;
			if (FAILED(dx->device->GetTexture(slot, &current)) || !current)
				continue;

			if (current == texture)
			{
				dx->device->SetTexture(slot, nullptr);
				bound |= 1u << slot;
			}
			current->Release();
		}
		return bound;
	}

	// A portal with nothing on the far side reads as a black hole behind the lip, the way an unlinked
	// one does in Portal. It is also the floor the live views are written over each frame.
	void GPortal::Blank()
	{
		IDirect3DTexture9* black = rgp->blackImage ? rgp->blackImage->texture.map : nullptr;
		if (!black)
			return;

		for (auto& surface : Surfaces)
			surface.Image->texture.map = black;
	}

	void GPortal::Bind(const PortalTarget& target, const PortalEndpoint& endpoint)
	{
		const auto texture = std::static_pointer_cast<DX9Texture>(target.Color);
		if (texture && texture->Data)
			Assign(endpoint.Surface, texture->Data);
	}

	void GPortal::Assign(Material* material, IDirect3DTexture9* texture)
	{
		for (auto& surface : Surfaces)
		{
			if (surface.Material == material)
				surface.Image->texture.map = texture;
		}
	}

	void GPortal::Restore()
	{
		for (auto& surface : Surfaces)
		{
			if (surface.Image && surface.Original)
				surface.Image->texture.map = surface.Original;
		}
	}

	void GPortal::DrawDebug()
	{
		if (!Debug || !Debug->current.enabled)
			return;

		// Magenta and clear of the run HUD: the portals themselves are orange and blue, and the mid
		// left of the screen is usually covered by one of them.
		static GText text{ "", FONT_NORMAL, 10, 120, 1.2f, vec4(1, 0, 1, 1) };

		text.Value = std::format("portal: {}\nframes {}  mats {}  surfaces {}\nquads {}  paired {}  captured {}\n{}  {}",
			DebugStage, DebugFrames, rgp ? rgp->materialCount : -1, Surfaces.size(), DebugModels, DebugMatched,
			DebugCaptured, DebugMaterial, DebugTechniques);
		text.Render();
	}

}
