#include "PMove.hpp"

namespace IW3SR
{
	void PMove::FinishMove(usercmd_s* cmd)
	{
		if (cgs->predictedPlayerState.pm_type != PM_NORMAL)
			return CL_FinishMove_h(cmd);

		CL_FinishMove_h(cmd);

		EventPMoveFinish event(cmd, &cgs->predictedPlayerState);
		Application::Dispatch(event);
	}

	void PMove::WalkMove(pmove_t* pm, pml_t* pml)
	{
		EventPMoveWalk event(pm, pml);
		Application::Dispatch(event);

		if (!event.PreventDefault)
			PM_WalkMove_h(pm, pml);
	}

	void PMove::AirMove(pmove_t* pm, pml_t* pml)
	{
		EventPMoveAir event(pm, pml);
		Application::Dispatch(event);

		if (!event.PreventDefault)
			PM_AirMove_h(pm, pml);
	}

	void PMove::GroundTrace(pmove_t* pm, pml_t* pml)
	{
		EventPMoveGroundTrace event(pm, pml);
		Application::Dispatch(event);

		if (!event.PreventDefault)
			PM_GroundTrace_h(pm, pml);
	}

	void PMove::SetYaw(usercmd_s* cmd, const vec3& target)
	{
		auto deltas = cgs->predictedPlayerState.delta_angles;
		float angle = SHORT2ANGLE(cmd->angles[1]);

		float input = Math::AngleDelta(Math::AngleDelta(deltas[1], angle), deltas[1]);
		float to = Math::AngleDelta(deltas[1], target[1]);
		float delta = input - to;

		clients->viewangles[1] += delta;
		cmd->angles[1] += ANGLE2SHORT(delta);
	}

	void PMove::SetPitch(usercmd_s* cmd, const vec3& target)
	{
		auto deltas = cgs->predictedPlayerState.delta_angles;
		float angle = SHORT2ANGLE(cmd->angles[0]);

		float input = Math::AngleDelta(Math::AngleDelta(deltas[0], angle), deltas[0]);
		float to = Math::AngleDelta(deltas[0], target[0]);
		float delta = input - to;

		clients->viewangles[0] += delta;
		cmd->angles[0] += ANGLE2SHORT(delta);
	}

	void PMove::SetRoll(usercmd_s* cmd, const vec3& target)
	{
		auto deltas = cgs->predictedPlayerState.delta_angles;
		float angle = SHORT2ANGLE(cmd->angles[2]);

		float input = Math::AngleDelta(Math::AngleDelta(deltas[2], angle), deltas[2]);
		float to = Math::AngleDelta(deltas[2], target[2]);
		float delta = input - to;

		clients->viewangles[2] += delta;
		cmd->angles[2] += ANGLE2SHORT(delta);
	}

	void PMove::SetAngles(usercmd_s* cmd, const vec3& target)
	{
		SetPitch(cmd, target);
		SetYaw(cmd, target);
		SetRoll(cmd, target);
	}

	pmove_t PMove::CreatePmove(playerState_s* ps, usercmd_s* cmd)
	{
		usercmd_s* oldcmd = GetUserCommand(clients->cmdNumber - 1);
		pmove_t pm = { 0 };

		pm.ps = ps;
		pm.cmd = *cmd;
		pm.oldcmd = *oldcmd;

		pm.mins[0] = -15;
		pm.mins[1] = -15;
		pm.mins[2] = 0;
		pm.maxs[0] = 15;
		pm.maxs[1] = 15;
		if (ps->viewHeightCurrent == 60)
			pm.maxs[2] = 70;
		if (ps->viewHeightCurrent == 40)
			pm.maxs[2] = 50;
		if (ps->viewHeightCurrent == 11)
			pm.maxs[2] = 30;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.handler = 1;

		return pm;
	}

	void PMove::PredictPmoveSingle(pmove_t* pm, int amount)
	{
		Memory::Write(0x537D10, "\xC3");
		for (int i = 0; i < amount; i++)
			PmoveSingle(pm);
		Memory::Write(0x537D10, "\x81");
	}

	usercmd_s* PMove::GetUserCommand(int cmdNumber)
	{
		return &clients->cmds[cmdNumber & 0x7F];
	}

	vec3 PMove::GetEyePos()
	{
		vec3 org = cgs->predictedPlayerState.origin;
		org.z += cgs->predictedPlayerState.viewHeightCurrent;
		return org;
	}

	void PMove::DisableSprint(playerState_s* ps)
	{
		ps->sprintState.sprintButtonUpRequired = 1;
	}

	bool PMove::OnGround()
	{
		return cgs->predictedPlayerState.groundEntityNum != ENTITYNUM_NONE;
	}
}
