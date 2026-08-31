#pragma once
#include <cstdint>

namespace IW3SR
{
	// A vanilla frame limiter kept running on a clock of its own. Com_Frame leaves NET_Sleep(1) the
	// moment the elapsed millisecond count reaches 1000/com_maxfps, so a sleep costing more than a
	// millisecond overshoots and real frames come out a mix of widths just at and just over the
	// target. Replaying that here is what keeps the movement rate one a client at that com_maxfps
	// could actually have reached, rather than the rate the number names.
	struct Cadence
	{
		int64_t Micros = 0;
		int Frame = 0;
		int Last = 0;
		bool Started = false;
	};

	// Microseconds a one millisecond sleep and a frame's own work cost. The overshoot is entirely
	// these two numbers, so they are measured rather than assumed.
	struct Pacing
	{
		int Sleep = 1000;
		int Work = 500;
	};

	// Steps run from Clock, each as wide as its entry in the widths array.
	struct Steps
	{
		int Clock = 0;
		int Count = 0;
		bool Starved = false;
	};

	// Deliberately free of engine state so the stepping can be exercised without the game running.
	// frametime is the width of the render frame this is called from: the cadence seeds a frame
	// behind the caller, or it would owe nothing on the frame it starts and stay a step short.
	Steps PlanSteps(Cadence& cadence, int* widths, int limit, int step, int target, int frametime, int maxDrift,
		const Pacing& pacing);
}
