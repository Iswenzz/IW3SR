#pragma once

namespace IW3SR
{
	// How much of a frame's elapsed time turns into fixed size movement steps. Steps run from
	// Clock, at Clock + step through Clock + Count * step.
	struct Steps
	{
		int Clock = 0;
		int Count = 0;
		bool Starved = false;
	};

	// Deliberately free of engine state so the stepping can be exercised without the game running.
	Steps PlanSteps(int clock, int target, int step, int maxSteps, int maxDrift);
}
