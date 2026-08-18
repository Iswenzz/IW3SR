#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API CoD4
	{
	public:
		static float CmdScale(playerState_s* ps, usercmd_s* cmd);
		static void ProjectVelocity(const vec3& in, const vec3& normal, vec3& out);
		static void JumpClearState(playerState_s* ps);
		static bool InKnockback(playerState_s* ps);
	};
}
