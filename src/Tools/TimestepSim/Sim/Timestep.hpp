#pragma once
// The mod's command splitting, driven off a simulated client instead of the game's globals. The
// cadence itself is the shipping PlanSteps, compiled straight out of src, and the rest mirrors
// IW3SR::Timestep::Split line for line, so what runs here is what runs in the game.
#include "Engine/Client.hpp"
#include "Engine/Com.hpp"
#include "Game/System/Schedule.hpp"

namespace Sim
{
	enum class SplitMode
	{
		// What the mod did before: whole steps of 1000/com_maxfps on an even grid, with the
		// remainder carried. Delivers the rate com_maxfps names, which a real client cannot.
		// Kept only so the regression can still show what that costs; the game has no such mode.
		Grid,
		// What the mod does: widths from a vanilla frame limiter run against the same elapsed
		// time, so the cadence carries the overshoot a real client would have had.
		Limiter,
	};

	struct SplitStats
	{
		int emitted = 0;
		int starved = 0;
		int clock = 0;
	};

	int SplitCommands(ClientState& cl, int& movementClock, int movementFps, SplitMode mode, IW3SR::Cadence& cadence,
		const Pacing& pacing, SplitStats& stats);
}
