#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	// Shared pmove replay for the movement HUDs. Runs the accelerate step of the active movement
	// mode against a copy of the player state and records what it would have applied.
	class PmoveHud
	{
	protected:
		pmove_t pm = {};
		pml_t pml = {};
		vec2 w_vel = { 0, 0 };
		float w_speed = 0;
		float accelerate = 0;
		float slick_gravity = 0;

		bool Update();

		void WalkMove();
		void AirMove();
		void Accelerate(float wishspeed, float accel);
		void SlickAccelerate(float wishspeed, float accel);

		float CmdScale(playerState_s* ps, usercmd_s* cmd);
		float CmdScaleWalk(usercmd_s* cmd);
		float CmdScaleForStance();
		float DamageScaleWalk(int damageTimer);

		float GetViewHeightLerpTime(int iTarget, int bDown);
		float GetViewHeightLerp(int fromHeight, int toHeight);
	};
}
