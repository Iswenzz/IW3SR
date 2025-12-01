#pragma once
// https://github.com/xoxor4d/iw3xo-dev/blob/master/src/components/modules/movement.cpp
#include "Base.hpp"

namespace IW3SR::Addons
{
	class Q3
	{
	public:
		static void WalkMove(pmove_t* pm, pml_t* pml);
		static void AirMove(pmove_t* pm, pml_t* pml);
		static void GroundTrace(pmove_t* pm, pml_t* pml);
		static void AirControl(pmove_t* pm, pml_t* pml, const vec3& wishdir_b, float wishspeed_b);
		static void Accelerate(playerState_s* ps, pml_t* pml, const vec3& wishdir_b, float wishspeed_b, float accel_b);
		static void AccelerateWalk(const vec3& wishdir, pml_t* pml, playerState_s* ps, float wishspeed, float accel);
		static bool JumpCheck(pmove_t* pm, pml_t* pml);
		static void Friction(pmove_t* pm, pml_t* pml);
		static void ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce);
		static bool SlideMove(pmove_t* pm, pml_t* pml, bool gravity);
		static void StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity);
		static float CmdScale(playerState_s* ps, usercmd_s* cmd);
		static void SetMovementDir(pmove_t* pm);
	};
}
