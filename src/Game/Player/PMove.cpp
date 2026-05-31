#include "PMove.hpp"

#include "Game/Player/Movements/CS.hpp"
#include "Game/Player/Movements/Q3.hpp"

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
		switch (GetMovementMode())
		{
		case MovementMode::COD4:
			PM_WalkMove_h(pm, pml);
			break;
		case MovementMode::Q3:
			Q3::WalkMove(pm, pml);
			break;
		case MovementMode::Q3CPM:
			Q3::WalkMoveCPM(pm, pml);
			break;
		case MovementMode::CS:
			CS::WalkMove(pm, pml);
			break;
		}
		EventPMoveWalk event(pm, pml);
		Application::Dispatch(event);
	}

	void PMove::AirMove(pmove_t* pm, pml_t* pml)
	{
		switch (GetMovementMode())
		{
		case MovementMode::COD4:
			PM_AirMove_h(pm, pml);
			break;
		case MovementMode::Q3:
			Q3::AirMove(pm, pml);
			break;
		case MovementMode::Q3CPM:
			Q3::AirMoveCPM(pm, pml);
			break;
		case MovementMode::CS:
			CS::AirMove(pm, pml);
			break;
		}
		EventPMoveAir event(pm, pml);
		Application::Dispatch(event);
	}

	void PMove::GroundTrace(pmove_t* pm, pml_t* pml)
	{
		switch (GetMovementMode())
		{
		case MovementMode::COD4:
			PM_GroundTrace_h(pm, pml);
			break;
		case MovementMode::Q3:
		case MovementMode::Q3CPM:
			Q3::GroundTrace(pm, pml);
			break;
		case MovementMode::CS:
			CS::GroundTrace(pm, pml);
			break;
		}
		EventPMoveGroundTrace event(pm, pml);
		Application::Dispatch(event);
	}

	void PMove::CrashLand(playerState_s* ps, pml_t* pml)
	{
		EventPMoveCrashLand event(ps, pml);
		Application::Dispatch(event);
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

	MovementMode PMove::GetMovementMode()
	{
		switch (statData->stats.data.bytedata[1700]) // sr_mode
		{
		case 1: // 190
		case 2: // 210
			return MovementMode::COD4;
		case 3: // Q3
			return MovementMode::Q3;
		case 4: // Q3CPM
		case 5: // Q3CPMW
			return MovementMode::Q3CPM;
		case 6: // CS
		case 7: // Portal
			return MovementMode::CS;
		}
		return MovementMode::COD4;
	}

	vec3 PMove::GetEyePos()
	{
		vec3 org = cgs->predictedPlayerState.origin;
		org.z += cgs->predictedPlayerState.viewHeightCurrent;
		return org;
	}

	bool PMove::OnGround()
	{
		return cgs->predictedPlayerState.groundEntityNum != ENTITYNUM_NONE;
	}
}
