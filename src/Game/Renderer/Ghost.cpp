#include "Ghost.hpp"

#include "Game/System/ServerGhost.hpp"

namespace IW3SR
{
	namespace
	{
		// The engine's flag that keeps a model out of the sun's shadow maps, and one it never reads, which
		// stays with the model's entity in the frame's data all the way to the backend.
		constexpr uint32_t FlagNoSunShadow = 0x1;
		constexpr uint32_t FlagGhost = 0x4000;

		// rb_tessTable, one draw function for each surface type; the model types are 7 to 9.
		constexpr uintptr_t TessTable = 0x6B1AE0;
		constexpr int FirstModelType = 7;
		constexpr int ModelTypes = 3;

		constexpr int EntityLitList = 2; // DRAW_SURF_ENT_CAMERA_LIT
		constexpr uint64_t LastSortKey = 63;

		// GFXS0 fields, which hold the D3D values themselves.
		constexpr uint32_t BlendOpMask = 0x700;
		constexpr uint32_t SourceBlendMask = 0xF;
		constexpr uint32_t DestBlendMask = 0xF0;

		static_assert(offsetof(GfxBackEndData, gfxEnts) == 0x118260);
		static_assert(offsetof(GfxModelSkinnedSurface, info) + offsetof(GfxModelSurfaceInfo, gfxEntIndex) == 0xE);
		static_assert(offsetof(GfxCmdBufSourceState, input) + offsetof(GfxCmdBufInput, data) == 0xC28);
		static_assert(offsetof(GfxCmdBufState, technique) == 0xC0);

		using TessFunction = unsigned int (*)(const GfxDrawSurfListArgs* args, GfxCmdBufContext prepass);
		TessFunction Stock[ModelTypes] = {};

		bool IsModel(GfxDrawSurf surf)
		{
			const int type = static_cast<int>(surf.fields.surfType);
			return type >= FirstModelType && type < FirstModelType + ModelTypes;
		}

		// A model surface is found in the frame's data by its object id, and names its entity there.
		bool IsGhost(const GfxBackEndData* data, GfxDrawSurf surf)
		{
			const size_t offset = static_cast<size_t>(surf.fields.objectId) * 4;
			const auto* model =
				reinterpret_cast<const GfxModelSkinnedSurface*>(reinterpret_cast<const uint8_t*>(data) + offset);
			const int entity = model->info.gfxEntIndex;
			return entity && (data->gfxEnts[entity].renderFxFlags & FlagGhost);
		}

		// Every pass but those that fill the depth and shadow maps or add one light's share.
		bool DrawsColor(MaterialTechniqueType type)
		{
			if (type <= TECHNIQUE_BUILD_SHADOWMAP_COLOR)
				return false;
			if (type >= TECHNIQUE_LIGHT_SPOT && type <= TECHNIQUE_LIGHT_SPOT_SHADOW)
				return false;
			return type != TECHNIQUE_SHADOWCOOKIE_CASTER && type != TECHNIQUE_SHADOWCOOKIE_RECEIVER;
		}

		// The ghost's opaque surfaces blend at a constant factor, which asks nothing of the stock shaders.
		// The engine tracks none of it, so the blend state goes back to what it believes the device holds.
		// A surface that blends by itself is left to.
		unsigned int Draw(int type, const GfxDrawSurfListArgs* args, GfxCmdBufContext prepass)
		{
			const GfxDrawSurfListInfo* info = args->info;
			const GfxBackEndData* data = args->context.source->input.data;
			GfxCmdBufState* state = args->context.state;

			if (!IsGhost(data, info->drawSurfs[args->firstDrawSurfIndex]) || (state->refStateBits[0] & BlendOpMask))
				return Stock[type](args, prepass);
			if (!DrawsColor(info->baseTechType))
				return 1;

			IDirect3DDevice9* device = state->prim.device;
			const DWORD alpha = static_cast<DWORD>(std::clamp(GGhost::Opacity, 0.0f, 1.0f) * 255.0f + 0.5f);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
			device->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(alpha, alpha, alpha, alpha));

			const unsigned int drawn = Stock[type](args, prepass);

			const uint32_t active = state->activeStateBits[0];
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			if (active & BlendOpMask)
				device->SetRenderState(D3DRS_BLENDOP, (active & BlendOpMask) >> 8);
			if (active & SourceBlendMask)
				device->SetRenderState(D3DRS_SRCBLEND, active & SourceBlendMask);
			if (active & DestBlendMask)
				device->SetRenderState(D3DRS_DESTBLEND, (active & DestBlendMask) >> 4);
			return drawn;
		}

		unsigned int TessRigid(const GfxDrawSurfListArgs* args, GfxCmdBufContext prepass)
		{
			return Draw(0, args, prepass);
		}

		unsigned int TessRigidSkinned(const GfxDrawSurfListArgs* args, GfxCmdBufContext prepass)
		{
			return Draw(1, args, prepass);
		}

		unsigned int TessSkinned(const GfxDrawSurfListArgs* args, GfxCmdBufContext prepass)
		{
			return Draw(2, args, prepass);
		}

		constexpr TessFunction Wrappers[ModelTypes] = { TessRigid, TessRigidSkinned, TessSkinned };
	}

	// The backend draws every run of surfaces through its table, so the model entries are where the ghost's
	// are caught.
	void GGhost::Initialize()
	{
		if (Stock[0])
			return;

		for (int i = 0; i < ModelTypes; i++)
		{
			const uintptr_t entry = TessTable + (FirstModelType + i) * sizeof(TessFunction);
			Stock[i] = Memory::Get<TessFunction>(entry);
			Memory::Set<TessFunction>(entry, Wrappers[i]);
		}
	}

	// The render flags the ghost's model goes into the scene with. Any at all give it an entry in the
	// frame's data, which the backend then reads its surfaces by.
	uint32_t GGhost::SceneFlags(int entnum)
	{
		const ServerGhostState* ghost = ServerGhost::Get();
		if (!ghost || entnum != ghost->Slot || Opacity >= 1.0f)
			return 0;
		return FlagGhost | FlagNoSunShadow;
	}

	// The engine sorts an entity's surfaces among the opaque ones, ahead of the sky, where a blended surface
	// would mix with whatever the frame held before. The ghost's go last in the lit pass, in the order they
	// were in.
	void GGhost::SceneSurfaces(const GfxViewInfo* viewInfo)
	{
		R_AddAllSceneEntSurfacesCamera_h(viewInfo);

		const GfxBackEndData* data = *gfx_frontEndDataOut;
		GfxDrawSurf* const surfs = scene->drawSurfs[EntityLitList];
		const int count = scene->drawSurfCount[EntityLitList];
		if (!data || !surfs || count <= 0)
			return;

		GfxDrawSurf* const end = surfs + count;
		GfxDrawSurf* const first = std::stable_partition(surfs, end,
			[data](const GfxDrawSurf& surf) { return !IsModel(surf) || !IsGhost(data, surf); });

		for (GfxDrawSurf* surf = first; surf != end; surf++)
			surf->fields.primarySortKey = LastSortKey;
	}
}
