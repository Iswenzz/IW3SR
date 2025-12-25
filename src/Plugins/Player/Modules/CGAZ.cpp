#include "CGAZ.hpp"

#define pm_accelerate 9.0f
#define pm_crouch_accelerate 12.0f
#define pm_prone_accelerate 19.0f
#define pm_airaccelerate 1.0f

namespace IW3SR::Addons
{
	CGAZ::CGAZ() : Module("sr.player.cgaz", "Player", "CGAZ")
	{
		ColorBackground = { 0.25, 0.25, 0.25, 0.7 };
		ColorPartialAccel = { 0, 1, 0, 0.7 };
		ColorFullAccel = { 0, 0.25, 0.25, 0.7 };
		ColorTurnZone = { 1, 1, 0, 0.5 };

		Y = 238;
		Height = 8;

		UseGroundZones = true;
	}

	void CGAZ::Menu()
	{
		ImGui::Checkbox("Ground Zones", &UseGroundZones);

		ImGui::DragFloat("Y Position", &Y);
		ImGui::DragFloat("Height", &Height);

		ImGui::ColorEdit4("Background", &ColorBackground.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Partial Accel", &ColorPartialAccel.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Full Accel", &ColorFullAccel.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Turn Zone", &ColorTurnZone.x, ImGuiColorEditFlags_Float);
	}

	void CGAZ::PmoveSingle()
	{
		pml.frametime = static_cast<float>(cgs->frametime) / 1000.f;
		pml.previous_velocity = pm.ps->velocity;
		Math::AngleVectors(pm.ps->viewangles, pml.forward, pml.right, pml.up);

		// Use default key combination when no user input
		if (!pm.cmd.forwardmove && !pm.cmd.rightmove)
			pm.cmd.forwardmove = 127;

		// Trace
		PM_GroundTrace(&pm, &pml);

		if (pml.walking)
			WalkMove();
		else
			AirMove();
	}

	void CGAZ::WalkMove()
	{
		if (Jump_Check(&pm, &pml))
		{
			AirMove();
			return;
		}
		PM_Friction(pm.ps, &pml);

		// Project moves down to flat plane
		pml.forward[2] = 0;
		pml.right[2] = 0;

		pml.forward = glm::normalize(pml.forward);
		pml.right = glm::normalize(pml.right);

		for (int i = 0; i < 2; ++i)
			w_vel[i] = static_cast<float>(pm.cmd.forwardmove) * pml.forward[i]
				+ static_cast<float>(pm.cmd.rightmove) * pml.right[i];

		const float dmg_scale = DamageScaleWalk(pm.ps->damageTimer) * CmdScaleWalk(&pm.cmd);
		const float wishspeed = dmg_scale * glm::length(w_vel);

		// When a player gets hit, they temporarily lose full control, which allows them to be moved a bit
		if (pml.groundTrace.surfaceFlags & SURF_SLICK || pm.ps->pm_flags & PMF_TIME_KNOCKBACK)
			SlickAccelerate(wishspeed, pm_airaccelerate);
		else
		{
			float accelerate = pm_accelerate;
			if (pm.ps->viewHeightTarget == 11)
				accelerate = pm_prone_accelerate;
			else if (pm.ps->viewHeightTarget == 40)
				accelerate = pm_crouch_accelerate;
			Accelerate(wishspeed, accelerate);
		}
	}

	void CGAZ::AirMove()
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

		const float wishspeed = scale * glm::length(w_vel);
		Accelerate(wishspeed, pm_airaccelerate);
	}

	void CGAZ::Compute(float wishspeed, float accel, float gravity)
	{
		g_squared = gravity * gravity;
		v_squared = glm::dot(vec2(pml.previous_velocity), vec2(pml.previous_velocity));
		vf_squared = glm::dot(vec2(pm.ps->velocity), vec2(pm.ps->velocity));
		w_speed = wishspeed;
		a = accel * wishspeed * pml.frametime;
		a_squared = a * a;

		if (!UseGroundZones || v_squared - vf_squared >= 2 * a * wishspeed - a_squared)
			v_squared = vf_squared;

		v = sqrt(v_squared);
		vf = sqrt(vf_squared);

		d_min = Min();
		d_opt = Opt();
		d_max_cos = MaxCos(d_opt);
		d_max = Max(d_max_cos);
		d_vel = atan2(pm.ps->velocity[1], pm.ps->velocity[0]);
	}

	void CGAZ::Accelerate(float wishspeed, float accel)
	{
		Compute(wishspeed, accel, 0);
	}

	void CGAZ::SlickAccelerate(float wishspeed, float accel)
	{
		Compute(wishspeed, accel, static_cast<float>(pm.ps->gravity) * pml.frametime);
	}

	float CGAZ::CmdScale(playerState_s* ps, usercmd_s* cmd)
	{
		const auto player_spectateSpeedScale = Dvar::Get<float>("player_spectateSpeedScale");

		int max = abs(cmd->forwardmove);
		if (abs(cmd->rightmove) > max)
			max = abs(cmd->rightmove);
		if (!max)
			return 0.0f;

		float total = sqrt(static_cast<float>(cmd->rightmove * cmd->rightmove + cmd->forwardmove * cmd->forwardmove));
		float scale = static_cast<float>(ps->speed * max) / (total * 127.0f);

		if (ps->pm_flags & PMF_LEAN || 0.0f != ps->leanf)
			scale *= 0.4f;
		if (ps->pm_type == PM_NOCLIP)
			scale *= 3.0f;
		if (ps->pm_type == PM_UFO)
			scale *= 6.0f;
		if (ps->pm_type == PM_SPECTATOR)
			scale *= player_spectateSpeedScale;
		return scale;
	}

	float CGAZ::CmdScaleWalk(usercmd_s* cmd)
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
		if (pm.ps->pm_flags & PMF_LEAN || pm.ps->leanf != 0.0f || isProne)
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
			if (!pm.ps->weapon || weapon->moveSpeedScale <= 0.0f || pm.ps->pm_flags & PMF_LEAN || isProne)
			{
				if (pm.ps->weapon && weapon->adsMoveSpeedScale > 0.0f)
					scale = scale * weapon->adsMoveSpeedScale;
			}
			else
				scale = scale * weapon->moveSpeedScale;
		}
		return scale * pm.ps->moveSpeedScaleMultiplier;
	}

	float CGAZ::CmdScaleForStance()
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

	float CGAZ::DamageScaleWalk(int damageTimer)
	{
		const auto player_dmgtimer_maxTime = Dvar::Get<float>("player_dmgtimer_maxTime");
		const auto player_dmgtimer_minScale = Dvar::Get<float>("player_dmgtimer_minScale");

		if (!damageTimer || player_dmgtimer_maxTime == 0.0f)
			return 1.0f;
		return (-player_dmgtimer_minScale / player_dmgtimer_maxTime) * damageTimer + 1.0f;
	}

	float CGAZ::GetViewHeightLerpTime(int iTarget, int bDown)
	{
		if (iTarget == 11)
			return 400.0f;
		if (iTarget != 40)
			return 200.0f;
		if (bDown)
			return 200.0f;
		return 400.0f;
	}

	float CGAZ::GetViewHeightLerp(int fromHeight, int toHeight)
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

	float CGAZ::Min()
	{
		const float num_squared = w_speed * w_speed - v_squared + vf_squared + g_squared;
		const float num = sqrt(num_squared);
		return num >= vf ? 0 : acos(num / vf);
	}

	float CGAZ::Opt()
	{
		const float num = w_speed - a;
		return num >= vf ? 0 : acos(num / vf);
	}

	float CGAZ::MaxCos(float d_opt)
	{
		const float num = sqrt(v_squared - g_squared) - vf;
		float d_max_cos = num >= a ? 0 : acos(num / a);

		if (d_max_cos < d_opt)
			d_max_cos = d_opt;

		return d_max_cos;
	}

	float CGAZ::Max(float d_max_cos)
	{
		const float num = v_squared - vf_squared - a_squared - g_squared;
		const float den = 2 * a * vf;

		if (num >= den)
			return 0;
		if (-num >= den)
			return M_PI;

		float d_max = acos(num / den);
		if (d_max < d_max_cos)
			d_max = d_max_cos;

		return d_max;
	}

	void CGAZ::DrawAngleYaw(float start, float end, float yaw, const vec4& color)
	{
		const auto& scale = UI::Screen.VirtualToFull;
		const vec3 range = Math::AnglesToRange(start, end, yaw, cgs->refdef.tanHalfFovX);

		if (!range.z)
		{
			GDraw2D::Rect(rgp->whiteMaterial, vec2{ range.x, Y } * scale, vec2{ range.y - range.x, Height } * scale,
				color);
			return;
		}
		GDraw2D::Rect(rgp->whiteMaterial, vec2{ 0, Y } * scale, vec2{ range.x, Height } * scale, color);
		GDraw2D::Rect(rgp->whiteMaterial, vec2{ range.y, Y } * scale, vec2{ SCREEN_WIDTH - range.y, Height } * scale,
			color);
	}

	void CGAZ::OnDraw2D(EventRenderer2D& event)
	{
		pm = *pmove;

		if (!(pm.ps->otherFlags & PMF_DUCKED) && glm::length(vec2(pm.ps->velocity)))
		{
			PmoveSingle();

			const float yaw = atan2(w_vel[1], w_vel[0]) - d_vel;
			DrawAngleYaw(-d_min, +d_min, yaw, ColorBackground);
			DrawAngleYaw(+d_min, +d_opt, yaw, ColorPartialAccel);
			DrawAngleYaw(-d_opt, -d_min, yaw, ColorPartialAccel);
			DrawAngleYaw(+d_opt, +d_max_cos, yaw, ColorFullAccel);
			DrawAngleYaw(-d_max_cos, -d_opt, yaw, ColorFullAccel);
			DrawAngleYaw(+d_max_cos, +d_max, yaw, ColorTurnZone);
			DrawAngleYaw(-d_max, -d_max_cos, yaw, ColorTurnZone);
		}
	}
}
