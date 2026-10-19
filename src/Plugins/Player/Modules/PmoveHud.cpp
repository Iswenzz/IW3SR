#include "PmoveHud.hpp"

#include "Game/Player/Movements/CoD4.hpp"
#include "Game/System/Protocol.hpp"

#include <bit>

#define POF_FOLLOW 0x2

#define cod4_accelerate 9.0f
#define cod4_crouch_accelerate 12.0f
#define cod4_prone_accelerate 19.0f
#define cod4_airaccelerate 1.0f

#define q3_accelerate 10.0f
#define q3_crouch_accelerate 15.0f
#define q3_prone_accelerate 10.0f
#define q3_duck_scale 0.25f
#define q3_prone_scale 0.25f
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
		const bool following = cgs->predictedPlayerState.otherFlags & POF_FOLLOW;
		if (!pmove || (!pmove->ps && !following))
			return false;

		ps = following ? cgs->predictedPlayerState : *pmove->ps;
		pm = *pmove;
		pm.ps = &ps;
		demo = ServerDemo::Get();

		if (following)
			Follow();
		if (demo)
			ApplyDemo();
		if (!glm::length(vec2(pm.ps->velocity)))
			return false;

		w_speed = 0;
		accel_wishspeed = 0;
		accelerate = 0;
		slick_gravity = 0;

		pml.frametime = static_cast<float>(demo && demo->FPS > 0 ? 1000 / demo->FPS : cgs->frametime) / 1000.f;
		pml.previous_velocity = pm.ps->velocity;
		Math::AngleVectors(pm.ps->viewangles, pml.forward, pml.right, pml.up);

		// Use default key combination when no user input
		if (!pm.cmd.forwardmove && !pm.cmd.rightmove)
			pm.cmd.forwardmove = 127;

		PMove::TraceGround(&pm, &pml);

		if (pml.walking && !forceAir)
			WalkMove();
		else
			AirMove();

		return true;
	}

	// Following a player, the cgame runs no pmove, so cg_pmove still holds this client's own last command.
	// SR-Server copies each player's command into the dof fields of its state, which reach whoever follows
	// it; its sr_vrRelay is how a client tells it apart.
	void PmoveHud::Follow()
	{
		pm.cmd = {};
		if (GProtocol::SystemInfoValue("sr_vrRelay") == "1")
		{
			pm.cmd.forwardmove = static_cast<char>(std::bit_cast<uint32_t>(ps.dofNearStart));
			pm.cmd.rightmove = static_cast<char>(std::bit_cast<uint32_t>(ps.dofNearEnd));
			pm.cmd.buttons = std::bit_cast<int>(ps.dofFarStart);
		}
		pm.cmd.serverTime = ps.commandTime;

		// The command before it is unknown, so a held jump counts as held rather than pressed again.
		pm.oldcmd = pm.cmd;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.mins = { -15.0f, -15.0f, 0.0f };
		pm.maxs = { 15.0f, 15.0f, ps.viewHeightCurrent == 11 ? 30.0f : ps.viewHeightCurrent == 40 ? 50.0f : 70.0f };
	}

	// Watching a demo the server plays, the player is linked to the camera and pmove clears a linked player's
	// velocity, so the movement comes from the snapshots, which the cgame already interpolates when following,
	// and the keys with the demo's state.
	void PmoveHud::ApplyDemo()
	{
		if (!(ps.otherFlags & POF_FOLLOW) && cgs->snap && cgs->nextSnap)
			ps.velocity = glm::mix(cgs->snap->ps.velocity, cgs->nextSnap->ps.velocity, cgs->frameInterpolation);

		pm.cmd.forwardmove = static_cast<char>(demo->ForwardMove);
		pm.cmd.rightmove = static_cast<char>(demo->RightMove);
		pm.cmd.buttons = demo->Buttons;
		pm.oldcmd = pm.cmd;
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
		const float scale = mode == MovementMode::COD4 ? DamageScaleWalk(pm.ps->damageTimer) * CmdScaleWalk(&pm.cmd)
													   : CmdScale(pm.ps, &pm.cmd);
		float wishspeed = scale * glm::length(w_vel);

		if (is_q3)
		{
			const float speed = static_cast<float>(pm.ps->speed);
			if (pm.ps->pm_flags & PMF_DUCKED)
				wishspeed = std::min(wishspeed, speed * q3_duck_scale);
			else if (pm.ps->pm_flags & PMF_PRONE)
				wishspeed = std::min(wishspeed, speed * q3_prone_scale);
		}

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
		case MovementMode::Q3CPM:
			accelerate = mode == MovementMode::Q3CPM ? q3cpm_accelerate : q3_accelerate;
			if (pm.ps->pm_flags & PMF_DUCKED)
				accelerate = q3_crouch_accelerate;
			else if (pm.ps->pm_flags & PMF_PRONE)
				accelerate = q3_prone_accelerate;
			break;
		case MovementMode::CS:
			accelerate = cs_accelerate;
			break;
		}
		if (is_q3)
			AccelerateWalk(wishspeed, accelerate);
		else
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
		accel_wishspeed = wishspeed;
		accelerate = accel;
		slick_gravity = 0;
	}

	// Q3's ground move raises the wish speed its acceleration comes from to 100 when crouched or slower, or to
	// 80 when prone and faster, though it still caps at the real one.
	void PmoveHud::AccelerateWalk(float wishspeed, float accel)
	{
		Accelerate(wishspeed, accel);

		if (pm.ps->viewHeightCurrent == 40.0f || wishspeed < 100.0f)
			accel_wishspeed = 100.0f;
		else if (pm.ps->viewHeightCurrent == 11.0f)
			accel_wishspeed = 80.0f;
	}

	// The slick move loses the ground plane, so a frame of gravity comes with it
	void PmoveHud::SlickAccelerate(float wishspeed, float accel)
	{
		w_speed = wishspeed;
		accel_wishspeed = wishspeed;
		accelerate = accel;
		slick_gravity = static_cast<float>(pm.ps->gravity) * pml.frametime;
	}

	float PmoveHud::CmdScale(playerState_s* ps, usercmd_s* cmd)
	{
		static const auto player_spectateSpeedScale = Dvar::Find("player_spectateSpeedScale");

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
			scale *= player_spectateSpeedScale->current.value;
		return scale;
	}

	float PmoveHud::CmdScaleWalk(usercmd_s* cmd)
	{
		static const auto player_backSpeedScale = Dvar::Find("player_backSpeedScale");
		static const auto player_strafeSpeedScale = Dvar::Find("player_strafeSpeedScale");
		static const auto player_sprintSpeedScale = Dvar::Find("player_sprintSpeedScale");

		float total = sqrt(static_cast<float>(cmd->rightmove * cmd->rightmove + cmd->forwardmove * cmd->forwardmove));
		const bool isProne = pm.ps->pm_flags & PMF_PRONE && pm.ps->fWeaponPosFrac > 0.0f;

		float speed = fabs(static_cast<float>(cmd->forwardmove) * player_backSpeedScale->current.value);
		if (cmd->forwardmove >= 0)
			speed = fabs(static_cast<float>(cmd->forwardmove));
		if (speed - fabs(static_cast<float>(cmd->rightmove) * player_strafeSpeedScale->current.value) < 0.0f)
			speed = fabs(static_cast<float>(cmd->rightmove) * player_strafeSpeedScale->current.value);
		if (speed == 0.0f)
			return 0.0f;

		float scale = static_cast<float>(pm.ps->speed) * speed / (127.0f * total);
		if (pm.ps->pm_flags & PMF_WALKING || pm.ps->leanf != 0.0f || isProne)
			scale *= 0.40000001f;
		if (pm.ps->pm_flags & PMF_SPRINTING && pm.ps->viewHeightTarget == 60)
			scale *= player_sprintSpeedScale->current.value;
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
		static const auto player_dmgtimer_maxTime = Dvar::Find("player_dmgtimer_maxTime");
		static const auto player_dmgtimer_minScale = Dvar::Find("player_dmgtimer_minScale");

		if (!damageTimer || player_dmgtimer_maxTime->current.value == 0.0f)
			return 1.0f;
		return (-player_dmgtimer_minScale->current.value / player_dmgtimer_maxTime->current.value) * damageTimer + 1.0f;
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
