#pragma once
// https://github.com/xoxor4d/iw3xo-dev/blob/master/src/components/modules/movement.cpp
#include "CoD4.hpp"

namespace IW3SR
{
	class CS
	{
	public:
		static void WalkMove(pmove_t* pm, pml_t* pml);
		static void AirMove(pmove_t* pm, pml_t* pml);
		static void GroundTrace(pmove_t* pm, pml_t* pml);
		static bool JumpCheck(pmove_t* pm, pml_t* pml);
		static void Friction(pmove_t* pm, pml_t* pml);
		static void Accelerate(const vec3& wishdir, float wishspeed, playerState_s* ps, pml_t* pml);
		static void AirAccelerate(const vec3& wishdir, float wishspeed, playerState_s* ps, pml_t* pml);
		static void TryPlayerMove(pmove_t* pm, pml_t* pml);
		static void ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce);
		static float PermuteRestrictiveClipPlanes(const vec3& velocity, int planeCount, vec3* planes, int* permutation);
		static bool SlideMove(pmove_t* pm, pml_t* pml, bool gravity);
		static void StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity);
	};
}
