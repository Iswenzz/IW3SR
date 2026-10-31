#include "Lighting.hpp"

namespace IW3SR
{
	// Dynamic models get maxClientViews << 10 lighting entries, 1024 beside 3072 for static models, in a 256x256
	// image. A moving model holds three at once, so a busy map runs out and the frame drops the rest of its models.
	// << 12 is the engine's own sizing for four views: 4096 of each, in an image 512 tall.
	constexpr uintptr_t EntryShiftSite = 0x62F37B; // R_InitModelLightingGlobals

	// Cached static models carry their lighting coords as bytes the shaders scale by 1/256 whatever the image's
	// height, and the stock half block (2 * 256 / height) is lost at 512 tall. These make it (row * 4 + 2) / 2.
	constexpr uintptr_t PackedHalfBlockSite = 0x618728;		  // R_SkinXSurfaceStaticVerts: mov eax, 0x100
	constexpr uintptr_t PackedRowShiftSite = 0x618738;		  // shr ecx, 4
	constexpr uintptr_t PackedRowMaskSite = 0x61873B;		  // and cl, 0xFC
	constexpr uintptr_t PackedHalfBlockDoubleSite = 0x61874B; // add al, al

	// Each entry a frame lights queues one texture patch, into a list sized exactly for the stock 4096 entries and
	// fatal past them. Each back end buffer gets one sized for the new cache instead.
	constexpr uintptr_t AllocPatchAddress = 0x6265C0;	   // R_BackEndDataAllocAndClearModelLightingPatch
	constexpr uintptr_t PatchModelLightingSite = 0x61535B; // RB_BeginFrame
	constexpr uintptr_t BackEndDataAddress = 0xCC9F600;	   // s_backEndData[2]
	constexpr int MaxPatches = 8192;
	constexpr int ErrFatal = 0;

	static_assert(sizeof(GfxBackEndData) == 0x11E780);
	static_assert(offsetof(GfxBackEndData, modelLightingPatchCount) == 0x118228);
	static_assert(sizeof(GfxModelLightingPatch) == 0x28);

	namespace
	{
		GfxModelLightingPatch Patches[2][MaxPatches];

		GfxModelLightingPatch* PatchList(const GfxBackEndData* data)
		{
			return Patches[(reinterpret_cast<uintptr_t>(data) - BackEndDataAddress) / sizeof(GfxBackEndData)];
		}
	}

	void GLighting::Initialize()
	{
		Memory::Set<uint8_t>(EntryShiftSite, 12);

		Memory::Set<uint32_t>(PackedHalfBlockSite, 0x200);
		Memory::Set<uint8_t>(PackedRowShiftSite, 5);
		Memory::Set<uint8_t>(PackedRowMaskSite, 0xFE);
		Memory::NOP(PackedHalfBlockDoubleSite, 2);

		Memory::JMP(AllocPatchAddress, reinterpret_cast<uintptr_t>(&GLighting::AllocPatch));
		Memory::CALL(PatchModelLightingSite, reinterpret_cast<uintptr_t>(&GLighting::PatchModelLighting));
	}

	// Worker threads light models too, so the count is taken the way stock takes it.
	GfxModelLightingPatch* GLighting::AllocPatch()
	{
		GfxBackEndData* data = *gfx_frontEndDataOut;
		const LONG index = InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&data->modelLightingPatchCount), 1);
		if (index >= MaxPatches)
			Com_Error(ErrFatal, "%s", "modelLightingPatchList ran out of elements.");

		GfxModelLightingPatch* patch = &PatchList(data)[index];
		*patch = {};
		return patch;
	}

	void GLighting::PatchModelLighting(const GfxModelLightingPatch*, unsigned int patchCount)
	{
		RB_PatchModelLighting(PatchList(*gfx_backEndData), patchCount);
	}
}
