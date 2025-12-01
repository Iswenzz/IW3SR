#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API PMove
	{
	public:
		static usercmd_s* GetUserCommand(int cmdNumber);
		static vec3 GetEyePos();

		static void FinishMove(usercmd_s* cmd);
		static void WalkMove(pmove_t* pm, pml_t* pml);
		static void AirMove(pmove_t* pm, pml_t* pml);
		static void GroundTrace(pmove_t* pm, pml_t* pml);

		static void SetYaw(usercmd_s* cmd, const vec3& target);
		static void SetPitch(usercmd_s* cmd, const vec3& target);
		static void SetRoll(usercmd_s* cmd, const vec3& target);
		static void SetAngles(usercmd_s* cmd, const vec3& target);

		static void DisableSprint(playerState_s* ps);
		static bool OnGround();
		static bool InAir();
	};
}
