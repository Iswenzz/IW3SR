#include "PmoveHud.hpp"

#include "Game/Player/Movements/CoD4.hpp"

#define cod4_accelerate 9.0f
#define cod4_crouch_accelerate 12.0f
#define cod4_prone_accelerate 19.0f
#define cod4_airaccelerate 1.0f

#define q3_accelerate 10.0f
#define q3_airaccelerate 1.0f

#define q3cpm_accelerate 15.0f
#define q3cpm_airaccelerate 1.0f
#define q3cpm_strafeaccelerate 70.0f
#define q3cpm_airspeedcap 30.0f

#define cs_accelerate 10.0f
#define cs_airaccelerate 150.0f
#define cs_airspeedcap 30.0f

namespace IW3SR::Addons
{
	// Replays the accelerate step of the active movement mode without applying it, so the HUD
	// modules see the same wish direction and acceleration pmove is about to use
	bool PmoveHud::Update(bool forceAir)
	{
		if (!pmove || !pmove->ps)
			return false;

		pm = *pmove;

		if ((pm.ps->pm_flags & PMF_DUCKED) || !glm::length(vec2(pm.ps->velocity)))
			return false;

		w_speed = 0;
		accelerate = 0;
		slick_gravity = 0;

		pml.frametime = static_cast<float>(cgs->frametime) / 1000.f;
		pml.previous_velocity = pm.ps->velocity;
		Math::AngleVectors(pm.ps->viewangles, pml.forward, pml.right, pml.up);

		// Use default key combination when no user input
		if (!pm.cmd.forwardmove && !pm.cmd.rightmove)
			pm.cmd.forwardmove = 127;

		// Trace
		PM_GroundTrace(&pm, &pml);

		if (pml.walking && !forceAir)
			WalkMove();
		else
			AirMove();

		return true;
	}

	void PmoveHud::WalkMove()
	{
		if (Jump_Check(&pm, &pml))
		{
			AirMove();
			return;
		}
		const auto mode = PMove::GetMovementMode();
		const bool is_q3 = mode == MovementMode::Q3 || mode == MovementMode::Q3CPM;
		const bool is_slick = pml.groundTrace.surfaceFlags & SURF_SLICK;

		// Q3 drops friction entirely on slick surfaces and during knockback
		if (!is_q3 || !(is_slick || CoD4::InKnockback(pm.ps)))
			PM_Friction(pm.ps, &pml);

		// Project moves down to flat plane
		pml.forward[2] = 0;
		pml.right[2] = 0;

		pml.forward = glm::normalize(pml.forward);
		pml.right = glm::normalize(pml.right);

		for (int i = 0; i < 2; ++i)
			w_vel[i] = static_cast<float>(pm.cmd.forwardmove) * pml.forward[i]
				+ static_cast<float>(pm.cmd.rightmove) * pml.right[i];

		float accelerate = 0;
		// Only the CoD4 walk move scales by damage, stance, weapon and strafe direction
		const float scale = mode == MovementMode::COD4
			? DamageScaleWalk(pm.ps->damageTimer) * CmdScaleWalk(&pm.cmd)
			: CmdScale(pm.ps, &pm.cmd);
		const float wishspeed = scale * glm::length(w_vel);

		// When a player gets hit, they temporarily lose full control, which allows them to be moved a bit
		if ((is_slick || CoD4::InKnockback(pm.ps)) && mode != MovementMode::CS)
		{
			SlickAccelerate(wishspeed, mode == MovementMode::Q3CPM ? q3cpm_accelerate : cod4_airaccelerate);
			return;
		}
		switch (mode)
		{
		case MovementMode::COD4:
			accelerate = cod4_accelerate;
			if (pm.ps->viewHeightTarget == 11)
				accelerate = cod4_prone_accelerate;
			else if (pm.ps->viewHeightTarget == 40)
				accelerate = cod4_crouch_accelerate;
			break;
		case MovementMode::Q3:
			accelerate = q3_accelerate;
			break;
		case MovementMode::Q3CPM:
			accelerate = q3cpm_accelerate;
			break;
		case MovementMode::CS:
			accelerate = cs_accelerate;
			break;
		}
		Accelerate(wishspeed, accelerate);
	}

	void PmoveHud::AirMove()
	{
		PM_Friction(pm.ps, &pml);
		const float scale = CmdScale(pm.ps, &pm.cmd);

		// Project moves down to flat plane
		pml.forward[2] = 0;
		pml.right[2] = 0;

		pml.forward = glm::normalize(pml.forward);
		pml.right = glm::normalize(pml.right);

		for (int i = 0; i < 2; ++i)
			w_vel[i] = pm.cmd.forwardmove * pml.forward[i] + pm.cmd.rightmove * pml.right[i];

		float wishspeed = scale * glm::length(w_vel);
		float accel = 0;

		switch (PMove::GetMovementMode())
		{
		case MovementMode::COD4:
			accel = cod4_airaccelerate;
			break;
		case MovementMode::Q3:
			accel = q3_airaccelerate;
			break;
		case MovementMode::Q3CPM:
			accel = q3cpm_airaccelerate;
			if (pm.cmd.forwardmove == 0 && pm.cmd.rightmove != 0)
			{
				accel = q3cpm_strafeaccelerate;
				if (wishspeed > q3cpm_airspeedcap)
					wishspeed = q3cpm_airspeedcap;
			}
			break;
		case MovementMode::CS:
			accel = cs_airaccelerate;
			if (wishspeed > cs_airspeedcap)
				wishspeed = cs_airspeedcap;
			break;
		}
		Accelerate(wishspeed, accel);
	}

	void PmoveHud::Accelerate(float wishspeed, float accel)
	{
		w_speed = wishspeed;
		accelerate = accel;
		slick_gravity = 0;
	}

	// The slick move loses the ground plane, so a frame of gravity comes with it
	void PmoveHud::SlickAccelerate(float wishspeed, float accel)
	{
		w_speed = wishspeed;
		accelerate = accel;
		slick_gravity = static_cast<float>(pm.ps->gravity) * pml.frametime;
	}

	float PmoveHud::CmdScale(playerState_s* ps, usercmd_s* cmd)
	{
		const auto player_spectateSpeedScale = Dvar::Get<float>("player_spectateSpeedScale");

		int max = abs(cmd->forwardmove);
		if (abs(cmd->rightmove) > max)
			max = abs(cmd->rightmove);
		if (!max)
			return 0.0f;

		float total = sqrt(static_cast<float>(cmd->rightmove * cmd->rightmove + cmd->forwardmove * cmd->forwardmove));
		float scale = static_cast<float>(ps->speed * max) / (total * 127.0f);

		if (ps->pm_flags & PMF_WALKING || 0.0f != ps->leanf)
			scale *= 0.4f;
		if (ps->pm_type == PM_NOCLIP)
			scale *= 3.0f;
		if (ps->pm_type == PM_UFO)
			scale *= 6.0f;
		if (ps->pm_type == PM_SPECTATOR)
			scale *= player_spectateSpeedScale;
		return scale;
	}

	float PmoveHud::CmdScaleWalk(usercmd_s* cmd)
	{
		const auto player_backSpeedScale = Dvar::Get<float>("player_backSpeedScale");
		const auto player_strafeSpeedScale = Dvar::Get<float>("player_strafeSpeedScale");
		const auto player_sprintSpeedScale = Dvar::Get<float>("player_sprintSpeedScale");

		float total = sqrt(static_cast<float>(cmd->rightmove * cmd->rightmove + cmd->forwardmove * cmd->forwardmove));
		const bool isProne = pm.ps->pm_flags & PMF_PRONE && pm.ps->fWeaponPosFrac > 0.0f;

		float speed = fabs(static_cast<float>(cmd->forwardmove) * player_backSpeedScale);
		if (cmd->forwardmove >= 0)
			speed = fabs(static_cast<float>(cmd->forwardmove));
		if (speed - fabs(static_cast<float>(cmd->rightmove) * player_strafeSpeedScale) < 0.0f)
			speed = fabs(static_cast<float>(cmd->rightmove) * player_strafeSpeedScale);
		if (speed == 0.0f)
			return 0.0f;

		float scale = static_cast<float>(pm.ps->speed) * speed / (127.0f * total);
		if (pm.ps->pm_flags & PMF_WALKING || pm.ps->leanf != 0.0f || isProne)
			scale *= 0.40000001f;
		if (pm.ps->pm_flags & PMF_SPRINTING && pm.ps->viewHeightTarget == 60)
			scale *= player_sprintSpeedScale;
		if (pm.ps->pm_type == PM_NOCLIP)
			scale *= 3.0f;
		else if (pm.ps->pm_type == PM_UFO)
			scale *= 6.0f;
		else
			scale *= CmdScaleForStance();

		const auto weapon = bg_weaponNames[pm.ps->weapon];
		if (weapon)
		{
			if (!pm.ps->weapon || weapon->moveSpeedScale <= 0.0f || pm.ps->pm_flags & PMF_WALKING || isProne)
			{
				if (pm.ps->weapon && weapon->adsMoveSpeedScale > 0.0f)
					scale = scale * weapon->adsMoveSpeedScale;
			}
			else
				scale = scale * weapon->moveSpeedScale;
		}
		return scale * pm.ps->moveSpeedScaleMultiplier;
	}

	float PmoveHud::CmdScaleForStance()
	{
		float lerpFrac = GetViewHeightLerp(40, 11);
		if (lerpFrac != 0.0f)
			return 0.15000001f * lerpFrac + (1.0f - lerpFrac) * 0.64999998f;

		lerpFrac = GetViewHeightLerp(11, 40);
		if (lerpFrac != 0.0f)
			return 0.64999998f * lerpFrac + (1.0f - lerpFrac) * 0.15000001f;
		if (pm.ps->viewHeightTarget == 11)
			return 0.15000001f;
		if (pm.ps->viewHeightTarget == 22 || pm.ps->viewHeightTarget == 40)
			return 0.64999998f;
		return 1.0f;
	}

	float PmoveHud::DamageScaleWalk(int damageTimer)
	{
		const auto player_dmgtimer_maxTime = Dvar::Get<float>("player_dmgtimer_maxTime");
		const auto player_dmgtimer_minScale = Dvar::Get<float>("player_dmgtimer_minScale");

		if (!damageTimer || player_dmgtimer_maxTime == 0.0f)
			return 1.0f;
		return (-player_dmgtimer_minScale / player_dmgtimer_maxTime) * damageTimer + 1.0f;
	}

	float PmoveHud::GetViewHeightLerpTime(int iTarget, int bDown)
	{
		if (iTarget == 11)
			return 400.0f;
		if (iTarget != 40)
			return 200.0f;
		if (bDown)
			return 200.0f;
		return 400.0f;
	}

	float PmoveHud::GetViewHeightLerp(int fromHeight, int toHeight)
	{
		if (!pm.ps->viewHeightLerpTime)
			return 0.0f;

		if (fromHeight != -1 && toHeight != -1
			&& (toHeight != pm.ps->viewHeightLerpTarget
				|| toHeight == 40 && (fromHeight != 11 || pm.ps->viewHeightLerpDown)
					&& (fromHeight != 60 || !pm.ps->viewHeightLerpDown)))
			return 0.0f;

		float lerp_time = GetViewHeightLerpTime(pm.ps->viewHeightLerpTarget, pm.ps->viewHeightLerpDown);
		float flerp_frac = static_cast<float>(pm.cmd.serverTime - pm.ps->viewHeightLerpTime) / lerp_time;

		if (flerp_frac >= 0.0f)
		{
			if (flerp_frac > 1.0f)
				flerp_frac = 1.0f;
		}
		else
			flerp_frac = 0.0f;
		return flerp_frac;
	}

}
