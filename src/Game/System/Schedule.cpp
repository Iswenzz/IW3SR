#include "Schedule.hpp"

#include <cstdlib>

namespace IW3SR
{
	// One turn of Com_Frame's limiter on the virtual clock. Returns the msec it would have credited
	// the frame, which is the width of the movement step that frame owes.
	static int Advance(Cadence& cadence, int step, const Pacing& pacing)
	{
		cadence.Micros += pacing.Work;

		while (true)
		{
			cadence.Frame = static_cast<int>(cadence.Micros / 1000);

			// Guards a clock that ran backwards, which a virtual one never does.
			if (cadence.Last > cadence.Frame)
				cadence.Last = cadence.Frame;

			const int msec = cadence.Frame - cadence.Last;
			if (msec >= step)
			{
				cadence.Last = cadence.Frame;
				return msec;
			}
			cadence.Micros += pacing.Sleep;
		}
	}

	Steps PlanSteps(Cadence& cadence, int* widths, int limit, int step, int target, int frametime,
		int maxDrift, const Pacing& pacing)
	{
		if (step < 1)
			step = 1;

		// The virtual client starts a frame behind the real one rather than level with it, or it
		// would owe nothing on the frame it is seeded and stay a step short for the whole run. A
		// clock this far out is stale, from a map change or a server time correction.
		if (!cadence.Started || std::abs(target - cadence.Last) > maxDrift)
		{
			const int behind = target - (frametime > 0 ? frametime : step);

			cadence.Micros = static_cast<int64_t>(behind) * 1000;
			cadence.Frame = behind;
			cadence.Last = behind;
			cadence.Started = true;
		}

		Steps plan;
		while (plan.Count < limit && cadence.Micros / 1000 < target)
			widths[plan.Count++] = Advance(cadence, step, pacing);

		// More than a packet can carry. Spending the time is worse than queueing commands the
		// server would never see, but only one of the two keeps it agreeing with us.
		plan.Starved = plan.Count == limit && cadence.Micros / 1000 < target;

		// The caller advances the clock as it emits, so this is the first step's start.
		plan.Clock = cadence.Last;
		for (int i = 0; i < plan.Count; i++)
			plan.Clock -= widths[i];

		return plan;
	}
}
