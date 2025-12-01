#pragma once
// https://github.com/xoxor4d/iw3xo-dev/blob/master/src/components/modules/movement.cpp
#include "Base.hpp"

namespace IW3SR::Addons
{
	class CS
	{
	public:
		static void AirMove(pmove_t* pm, pml_t* pml);
		static void AirAccelerate(const vec3& wishdir, float wishspeed, playerState_s* ps, pml_t* pml);
		static void TryPlayerMove(pmove_t* pm, pml_t* pml);
		static void ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce);
	};
}
