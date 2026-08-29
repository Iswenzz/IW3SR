#include "Engine/Com.hpp"

namespace Sim
{
	// Com_ModifyMsec picks com_maxFrameTime as its ceiling only while a local server is up. A
	// client on a remote server, which is what the sim models, gets this instead.
	constexpr int ClampTime = 5000;

	int ComFrameLimiter(Clock& clock, int& comFrameTime, int& lastFrameTime, int maxFps, const Pacing& pacing)
	{
		// Integer division, so the cap a player types is rarely the rate they get: 125 lands on 8
		// ms exactly, 333 lands on 3 ms and therefore runs at 333.3.
		int minMsec = 1;
		if (maxFps > 0)
		{
			minMsec = 1000 / maxFps;
			if (!minMsec)
				minMsec = 1;
		}

		int64_t work = pacing.workMicros;
		if (work < pacing.minFrameMicros)
			work = pacing.minFrameMicros;
		clock.Advance(work);

		int msec;
		while (1)
		{
			comFrameTime = clock.Milliseconds();
			// Guards a clock that ran backwards, which a virtual one never does.
			if (lastFrameTime > comFrameTime)
				lastFrameTime = comFrameTime;
			msec = comFrameTime - lastFrameTime;
			if (msec >= minMsec)
				break;
			clock.Advance(pacing.sleepMicros);
		}
		lastFrameTime = comFrameTime;

		return msec;
	}

	int Com_ModifyMsec(int msec)
	{
		// com_fixedtime is 0 and com_timescale, com_codeTimeScale and dev_timescale are all 1 in
		// the sim, so the engine's scaling collapses to the two clamps.
		if (msec < 1)
			msec = 1;
		if (msec > ClampTime)
			msec = ClampTime;

		return msec;
	}
}
