#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// Draws the ghost SR-Server runs beside the player see-through: its surfaces go last in the lit pass,
	// blended at a constant factor, and stay out of the depth and shadow passes.
	class API GGhost
	{
	public:
		static inline float Opacity = 0.5f;

		static void Initialize();
		static uint32_t SceneFlags(int entnum);
		static void SceneSurfaces(const GfxViewInfo* viewInfo);
	};
}
