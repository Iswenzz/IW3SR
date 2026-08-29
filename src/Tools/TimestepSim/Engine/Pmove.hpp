#pragma once
// The engine half of movement: everything PmoveSingle does around the three functions IW3SR
// swaps out. WalkMove, AirMove and GroundTrace are dispatched through Movement so a run can pick
// COD4, Q3, Q3CPM or CS without the game's stat byte.
#include "Engine/Types.hpp"

namespace Sim
{
	using namespace IW3SR;

	enum class Mode
	{
		COD4,
		Q3,
		Q3CPM,
		CS,
	};

	void SetMode(Mode mode);
	Mode GetMode();

	// The engine's own movement, used for Mode::COD4 and as the reference the others replace.
	void PM_WalkMove(pmove_t* pm, pml_t* pml);
	void PM_AirMove(pmove_t* pm, pml_t* pml);
	void PM_GroundTrace(pmove_t* pm, pml_t* pml);
	void PM_Friction(playerState_s* ps, pml_t* pml);
	void PM_Accelerate(playerState_s* ps, const pml_t* pml, const vec3& wishdir, float wishspeed, float accel);
	void PM_StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity);
	bool PM_SlideMove(pmove_t* pm, pml_t* pml, bool gravity);
	void PM_ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce);

	// bg_jump.cpp. The button edge rule here is what decides whether a held jump rebounds.
	bool Jump_Check(pmove_t* pm, pml_t* pml);
	void Jump_ClearState(playerState_s* ps);

	void PmoveSingle(pmove_t* pm);
	void Pmove(pmove_t* pm);

	// Builds the pmove the client's prediction would build for one command.
	pmove_t MakePmove(playerState_s* ps, const usercmd_s& cmd, const usercmd_s& oldcmd);
}
