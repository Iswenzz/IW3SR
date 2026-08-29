#include <gtest/gtest.h>

#include <numeric>

#include "Game/System/Schedule.hpp"

using namespace IW3SR;

namespace
{
	constexpr int MaxSteps = 32;
	constexpr int MaxDrift = 500;

	// The frame limiter holds a frame to at least 1000/fps whole milliseconds, so a run of frames
	// advances the clock the way the engine does rather than in exact fractions.
	int Frame(int renderFps)
	{
		return 1000 / renderFps > 0 ? 1000 / renderFps : 1;
	}

	// A whole run: the widths every frame produced, and where the clock ended.
	struct Run
	{
		std::vector<int> widths;
		int clock = 0;
		int starved = 0;
	};

	Run Play(int movementFps, int renderFps, int duration, const Pacing& pacing)
	{
		const int step = 1000 / movementFps;
		const int frame = Frame(renderFps);

		Cadence cadence;
		Run run;

		for (int realtime = frame; realtime <= duration; realtime += frame)
		{
			int widths[MaxSteps] = {};
			const Steps plan = PlanSteps(cadence, widths, MaxSteps, step, realtime, frame, MaxDrift, pacing);

			run.starved += plan.Starved ? 1 : 0;
			run.clock = plan.Clock;

			for (int i = 0; i < plan.Count; i++)
			{
				run.widths.push_back(widths[i]);
				run.clock += widths[i];
			}
		}
		return run;
	}
}

// The whole claim the timestep makes: the movement rate follows com_maxfps and not the frame rate.
// Measured over a run rather than argued from the arithmetic.
TEST(Schedule, MovementRateIsIndependentOfFrameRate)
{
	const Pacing pacing = { 1200, 500 };

	for (const int movementFps : { 125, 250, 333, 500 })
	{
		const Run reference = Play(movementFps, movementFps, 4000, pacing);

		for (const int renderFps : { 30, 60, 100, 125, 144, 240, 333, 500 })
		{
			const Run run = Play(movementFps, renderFps, 4000, pacing);

			// Same number of steps as a client rendering at the movement rate, within one frame.
			EXPECT_NEAR(static_cast<int>(run.widths.size()), static_cast<int>(reference.widths.size()), 1)
				<< "movement " << movementFps << " drawn at " << renderFps;
		}
	}
}

// Steps carry the physics time that really elapsed. If they did not, movement would run slow or
// fast against the wall clock no matter how many of them there were.
TEST(Schedule, StepsCarryElapsedTime)
{
	const Pacing pacing = { 1200, 500 };

	for (const int movementFps : { 125, 250, 333, 1000 })
	{
		for (const int renderFps : { 60, 144, 333 })
		{
			const Run run = Play(movementFps, renderFps, 4000, pacing);
			const int carried = std::accumulate(run.widths.begin(), run.widths.end(), 0);

			EXPECT_NEAR(carried, 4000, 2 * Frame(renderFps))
				<< "movement " << movementFps << " drawn at " << renderFps;
		}
	}
}

// A step is never narrower than com_maxfps asks for. The limiter can only overshoot its target,
// never undershoot it, so a narrower step is one no vanilla client could have produced.
TEST(Schedule, NoStepIsNarrowerThanTheTarget)
{
	const Pacing pacing = { 1200, 500 };

	for (const int movementFps : { 76, 125, 250, 333, 1000 })
	{
		const int step = 1000 / movementFps;
		const Run run = Play(movementFps, 144, 4000, pacing);

		for (const int width : run.widths)
			EXPECT_GE(width, step) << "movement " << movementFps;
	}
}

// A sleep that costs exactly a millisecond is the only case where the limiter lands on its target,
// and it is the case an even grid would have been right for. Anything slower has to overshoot.
TEST(Schedule, OvershootFollowsWhatASleepCosts)
{
	const Run exact = Play(333, 144, 4000, { 1000, 500 });
	const Run slow = Play(333, 144, 4000, { 1200, 500 });

	for (const int width : exact.widths)
		EXPECT_EQ(width, 3);

	EXPECT_LT(slow.widths.size(), exact.widths.size());
	EXPECT_NE(*std::max_element(slow.widths.begin(), slow.widths.end()), 3);
}

// A rate that does not divide into a whole millisecond still has to average out to the rate the
// limiter would really have reached, rather than drifting away from the wall clock.
TEST(Schedule, HandlesRatesThatAreNotWholeMilliseconds)
{
	const Run run = Play(76, 144, 4000, { 1200, 500 });
	const int carried = std::accumulate(run.widths.begin(), run.widths.end(), 0);

	EXPECT_NEAR(carried, 4000, 20);
	for (const int width : run.widths)
		EXPECT_GE(width, 13);
}

// Past the drift window the clock is stale, from a map change or a server time correction, and has
// to resync rather than try to make up the whole gap a step at a time.
TEST(Schedule, ResyncsAfterALargeGap)
{
	const Pacing pacing = { 1200, 500 };
	Cadence cadence;
	int widths[MaxSteps] = {};

	PlanSteps(cadence, widths, MaxSteps, 3, 1000, 7, MaxDrift, pacing);
	const Steps jumped = PlanSteps(cadence, widths, MaxSteps, 3, 1000 + MaxDrift + 5000, 7, MaxDrift, pacing);

	EXPECT_LE(jumped.Count, 4);
	EXPECT_FALSE(jumped.Starved);
}

// A packet carries 32 commands. Past that the rate cannot be delivered, and saying so is what lets
// the client warn instead of silently running slow.
TEST(Schedule, ReportsStarvationPastAPacket)
{
	const Pacing pacing = { 1200, 500 };
	Cadence cadence;
	int widths[MaxSteps] = {};

	PlanSteps(cadence, widths, MaxSteps, 1, 1000, 1, MaxDrift, pacing);
	const Steps plan = PlanSteps(cadence, widths, MaxSteps, 1, 1400, 1, MaxDrift, pacing);

	EXPECT_EQ(plan.Count, MaxSteps);
	EXPECT_TRUE(plan.Starved);
}
