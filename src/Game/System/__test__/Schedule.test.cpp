// What is left here is what src/Tools/TimestepSim cannot reach. The simulator runs the cadence
// end to end against the real pmove and covers rate, carried time and overshoot far better than a
// unit test could; its clock is monotonic and smooth, so it never produces a stale clock or
// saturates a packet. Those two paths, and the one invariant they both rest on, live here.
#include <gtest/gtest.h>

#include "Game/System/Schedule.hpp"

using namespace IW3SR;

namespace
{
	constexpr int MaxSteps = 32;
	constexpr int MaxDrift = 500;

	// A machine whose one millisecond sleep really costs 1.2 ms, which is what makes a vanilla
	// limiter overshoot its target at all.
	constexpr Pacing Slow = { 1200, 500 };
}

// A step is never narrower than com_maxfps asks for. The limiter can only overshoot its target,
// never undershoot it, so a narrower step is one no vanilla client could have produced - which is
// the whole property the timestep exists to hold.
TEST(Schedule, NoStepIsNarrowerThanTheTarget)
{
	for (const int movementFps : { 76, 125, 250, 333, 1000 })
	{
		const int step = 1000 / movementFps;
		const int frame = 1000 / 144;

		Cadence cadence;
		for (int realtime = frame; realtime <= 4000; realtime += frame)
		{
			int widths[MaxSteps] = {};
			const Steps plan = PlanSteps(cadence, widths, MaxSteps, step, realtime, frame, MaxDrift, Slow);

			for (int i = 0; i < plan.Count; i++)
				EXPECT_GE(widths[i], step) << "com_maxfps " << movementFps;
		}
	}
}

// Past the drift window the clock is stale, from a map change or a server time correction. It has
// to resync rather than try to make the whole gap up a step at a time, which would spend the next
// several frames delivering movement that already happened.
TEST(Schedule, ResyncsAfterALargeGap)
{
	Cadence cadence;
	int widths[MaxSteps] = {};

	PlanSteps(cadence, widths, MaxSteps, 3, 1000, 7, MaxDrift, Slow);
	const Steps jumped = PlanSteps(cadence, widths, MaxSteps, 3, 1000 + MaxDrift + 5000, 7, MaxDrift, Slow);

	EXPECT_LE(jumped.Count, 4);
	EXPECT_FALSE(jumped.Starved);
}

// A packet carries 32 commands. Past that the rate cannot be delivered, and saying so is what lets
// the client warn instead of silently running slow.
TEST(Schedule, ReportsStarvationPastAPacket)
{
	Cadence cadence;
	int widths[MaxSteps] = {};

	PlanSteps(cadence, widths, MaxSteps, 1, 1000, 1, MaxDrift, Slow);
	const Steps plan = PlanSteps(cadence, widths, MaxSteps, 1, 1400, 1, MaxDrift, Slow);

	EXPECT_EQ(plan.Count, MaxSteps);
	EXPECT_TRUE(plan.Starved);
}
