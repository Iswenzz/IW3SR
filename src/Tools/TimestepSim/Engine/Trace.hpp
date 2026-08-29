#pragma once
// The world the simulator moves through. A flat floor with optional walls is enough to reproduce
// a hop chain, and it keeps a run reproducible without a BSP or any of the game's collision.
#include "Engine/Types.hpp"

#include <vector>

namespace Sim
{
	using namespace IW3SR;

	// An axis aligned box of solid. The floor is one of these.
	struct Brush
	{
		vec3 mins;
		vec3 maxs;
		int surfaceFlags = 0;
	};

	struct World
	{
		std::vector<Brush> brushes;

		static World Flat();
	};

	void SetWorld(const World* world);
	const World* GetWorld();
}
