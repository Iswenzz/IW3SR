#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	class CoD4
	{
	public:
		static float CmdScale(playerState_s* ps, usercmd_s* cmd);
		static void ProjectVelocity(const vec3& in, const vec3& normal, vec3& out);
		static void JumpClearState(playerState_s* ps);
	};
}
