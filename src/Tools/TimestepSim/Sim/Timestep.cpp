#include "Sim/Timestep.hpp"

#include <cmath>

namespace Sim
{
	// A packet carries at most 32 commands, so this is the same hard ceiling the mod works under.
	constexpr int MaxSteps = 32;
	constexpr int MaxDrift = 500;

	// The even grid the mod used to lay down, kept here and nowhere else. It is what the tool
	// compares against, and keeping it only in the tool is what stops it being reachable in game.
	static IW3SR::Steps PlanGrid(int* widths, int limit, int step, int target, int& clock)
	{
		const int elapsed = target - clock;
		if (!clock || elapsed < 0 || elapsed > MaxDrift)
			clock = target - step;

		IW3SR::Steps plan;
		plan.Clock = clock;
		plan.Count = (target - clock) / step;

		if (plan.Count > limit)
		{
			plan.Count = limit;
			plan.Clock = target - limit * step;
			plan.Starved = true;
		}
		if (plan.Count < 0)
			plan.Count = 0;

		for (int i = 0; i < plan.Count; i++)
			widths[i] = step;

		return plan;
	}

	int SplitCommands(ClientState& cl, int& movementClock, int movementFps, SplitMode mode, IW3SR::Cadence& cadence,
		const Pacing& pacing, SplitStats& stats)
	{
		const int step = 1000 / movementFps;

		const int target = cl.realtime;
		const int delta = cl.serverTime - target;

		int widths[MaxSteps] = {};
		IW3SR::Steps plan;

		if (mode == SplitMode::Grid)
		{
			plan = PlanGrid(widths, MaxSteps, step, target, movementClock);
		}
		else
		{
			const IW3SR::Pacing pace = { static_cast<int>(pacing.sleepMicros), static_cast<int>(pacing.workMicros) };
			plan = IW3SR::PlanSteps(cadence, widths, MaxSteps, step, target, cl.frametime, MaxDrift, pace);
		}
		const int count = plan.Count;
		movementClock = plan.Clock;

		stats.starved += plan.Starved ? 1 : 0;

		if (!count)
			return 0;

		const int serverTime = cl.serverTime;
		const int frameTime = cl.com_frameTime;
		const int frametime = cl.frametime;
		const unsigned int msec = cl.frame_msec;

		const int dx = cl.mouseDx[cl.mouseIndex];
		const int dy = cl.mouseDy[cl.mouseIndex];

		int time = movementClock;
		for (int i = 1; i <= count; i++)
		{
			const int width = widths[i - 1];
			time += width;

			cl.com_frameTime = time;
			cl.serverTime = time + delta;
			cl.frametime = width;
			cl.frame_msec = width;

			cl.mouseDx[cl.mouseIndex] = dx * i / count - dx * (i - 1) / count;
			cl.mouseDy[cl.mouseIndex] = dy * i / count - dy * (i - 1) / count;

			CL_CreateNewCommands(cl);
		}

		movementClock = time;
		stats.emitted += count;
		stats.clock = time;

		cl.serverTime = serverTime;
		cl.com_frameTime = frameTime;
		cl.frametime = frametime;
		cl.frame_msec = msec;

		return count;
	}
}
