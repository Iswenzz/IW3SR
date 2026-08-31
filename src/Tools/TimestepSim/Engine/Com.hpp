#pragma once
// The frame pacing half of Com_Frame. The simulator never sleeps for real: a virtual clock is
// advanced by the work a frame costs and by whatever NET_Sleep(1) would have cost, which is what
// makes a vanilla client's frame widths come out jittery rather than ideal.
#include <cstdint>

namespace Sim
{
	// Virtual Sys_Milliseconds. Kept in microseconds so sleep granularity can be modelled below a
	// millisecond, then truncated on read exactly as the engine's millisecond clock is.
	struct Clock
	{
		int64_t micros = 0;

		int Milliseconds() const
		{
			return static_cast<int>(micros / 1000);
		}
		void Advance(int64_t us)
		{
			micros += us;
		}
	};

	// How a frame is allowed to overshoot its target. Real hardware never lands on the cap
	// exactly; these are the two knobs that decide by how much it misses.
	struct Pacing
	{
		// What the frame's own work costs before the limiter is consulted.
		int64_t workMicros = 500;
		// What NET_Sleep(1) actually costs. Windows rounds a 1 ms sleep up.
		int64_t sleepMicros = 1000;
		// Set to model a machine that cannot reach the cap at all: the work cost is raised to this.
		int64_t minFrameMicros = 0;
	};

	// Com_Frame's limiter loop. Advances the clock past the frame's work and then over as many
	// NET_Sleep(1) calls as it takes for the elapsed millisecond count to reach minMsec, and
	// returns the msec the frame is credited with, exactly as the engine computes it.
	int ComFrameLimiter(Clock& clock, int& comFrameTime, int& lastFrameTime, int maxFps, const Pacing& pacing);

	int Com_ModifyMsec(int msec);
}
