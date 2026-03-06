#include "StrafeBot.hpp"

#define PM_AIRACCELERATE 1.0f

namespace IW3SR::Addons
{
	StrafeBot::StrafeBot() : Module("sr.player.strafebot", "Player", "Strafe Bot")
	{
		UseLatchSide = false;
		UseReleaseToStop = false;
		UseGroundStrafe = false;
		Side = 1;
		Mode = StrafeBotMode::WStrafe;
	}

	void StrafeBot::Menu()
	{
		ImGui::Checkbox("Ground Strafe", &UseGroundStrafe);
		ImGui::Tooltip("Steer view toward velocity direction when holding W+A or W+D on the ground.");

		ImGui::Checkbox("Latch Side", &UseLatchSide);
		ImGui::Tooltip("Lock strafe side at takeoff and don't change it mid-air.");

		if (UseLatchSide)
		{
			ImGui::Indent();
			ImGui::Checkbox("Release to Stop", &UseReleaseToStop);
			ImGui::Tooltip("Let go of A or D mid-air to stop steering and regain free mouse look.");
			ImGui::Unindent();
		}
	}

	float StrafeBot::ComputeOptimalYaw(playerState_s* ps, int side) const
	{
		const vec2 vel2d(ps->velocity[0], ps->velocity[1]);
		const float v = glm::length(vel2d);
		if (v < 1.0f)
			return std::numeric_limits<float>::quiet_NaN();

		const float wishspeed = static_cast<float>(ps->speed);
		const float frametime = static_cast<float>(cgs->frametime) / 1000.f;
		const float a = PM_AIRACCELERATE * wishspeed * frametime;
		const float threshold = wishspeed - a;

		if (v <= threshold)
			return std::numeric_limits<float>::quiet_NaN();

		const float d_opt = acosf(threshold / v);
		const float vel_yaw = atan2f(vel2d.y, vel2d.x) * (180.0f / static_cast<float>(M_PI));
		const float wish_yaw = vel_yaw - static_cast<float>(side) * (d_opt * (180.0f / static_cast<float>(M_PI)));

		float offset = (Mode == StrafeBotMode::WStrafe) ? 45.0f : 90.0f;
		return wish_yaw + static_cast<float>(side) * offset;
	}

	int StrafeBot::AutoDetectSide(playerState_s* ps, usercmd_s* cmd) const
	{
		if (cmd->rightmove < 0)
			return -1;
		if (cmd->rightmove > 0)
			return 1;
		return Side;
	}

	void StrafeBot::OnFinishMove(EventPMoveFinish& event)
	{
		playerState_s* ps = event.ps;
		usercmd_s* cmd = event.cmd;

		if (!ps || !cmd || ps->pm_type != PM_NORMAL)
			return;

		const bool airborne = (ps->groundEntityNum == ENTITYNUM_NONE);
		const bool justTookOff = airborne && !wasAirborne;
		wasAirborne = airborne;

		if (!airborne)
		{
			if (!UseGroundStrafe || cmd->forwardmove == 0 || cmd->rightmove == 0)
				return;

			const vec2 vel2d(ps->velocity[0], ps->velocity[1]);
			if (glm::length(vel2d) > 1.0f)
				PMove::SetYaw(cmd, vec3(0.0f, atan2f(vel2d.y, vel2d.x) * (180.0f / static_cast<float>(M_PI)), 0.0f));
			return;
		}

		if (justTookOff)
			Mode = (cmd->forwardmove != 0) ? StrafeBotMode::WStrafe : StrafeBotMode::StrafeOnly;

		if (!UseLatchSide || justTookOff)
			Side = AutoDetectSide(ps, cmd);

		if (UseLatchSide && UseReleaseToStop && cmd->rightmove != static_cast<char>(Side * 127))
			return;

		const float targetYaw = ComputeOptimalYaw(ps, Side);
		if (std::isnan(targetYaw))
			return;

		PMove::SetYaw(cmd, vec3(0.0f, targetYaw, 0.0f));
		cmd->forwardmove = (Mode == StrafeBotMode::WStrafe) ? 127 : 0;
		cmd->rightmove = static_cast<char>(Side * 127);
	}
}
