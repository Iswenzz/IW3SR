#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API GLighting
	{
	public:
		static void Initialize();

	private:
		static GfxModelLightingPatch* AllocPatch();
		static void PatchModelLighting(const GfxModelLightingPatch* patchList, unsigned int patchCount);
	};
}
