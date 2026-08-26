#include "Schedule.hpp"

namespace IW3SR
{
	// Divides the time since the last command into whole steps and carries the remainder, so the
	// movement clock tracks the frame clock without ever running ahead of it.
	Steps PlanSteps(int clock, int target, int step, int maxSteps, int maxDrift)
	{
		const int elapsed = target - clock;

		// A clock this far out is stale, from a map change or a server time correction.
		if (!clock || elapsed < 0 || elapsed > maxDrift)
			clock = target - step;

		Steps plan;
		plan.Clock = clock;
		plan.Count = (target - clock) / step;

		// More than a packet can carry. Spending the time is worse than queueing commands the
		// server would never see, but only one of the two keeps it agreeing with us.
		if (plan.Count > maxSteps)
		{
			plan.Count = maxSteps;
			plan.Clock = target - maxSteps * step;
			plan.Starved = true;
		}
		if (plan.Count < 0)
			plan.Count = 0;

		return plan;
	}
}
