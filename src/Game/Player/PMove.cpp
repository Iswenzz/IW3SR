#include "PMove.hpp"

namespace IW3SR
{
	void PMove::FinishMove(usercmd_s* cmd)
	{
		CL_FinishMove_h(cmd);

		EventPMoveFinish event(cmd);
		Application::Get().Dispatch(event);
	}

	void PMove::WalkMove(pmove_t* pm, pml_t* pml)
	{
		EventPMoveWalk event(pm, pml);
		Application::Get().Dispatch(event);

		if (!event.PreventDefault)
			PM_WalkMove_h(pm, pml);
	}

	void PMove::AirMove(pmove_t* pm, pml_t* pml)
	{
		EventPMoveAir event(pm, pml);
		Application::Get().Dispatch(event);

		if (!event.PreventDefault)
			PM_AirMove_h(pm, pml);
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
		SetYaw(cmd, target);
		SetPitch(cmd, target);
		SetRoll(cmd, target);
	}

	void PMove::DisableSprint(playerState_s* ps)
	{
		ps->sprintState.sprintButtonUpRequired = 1;
	}

	bool PMove::OnGround()
	{
		return cgs->predictedPlayerState.groundEntityNum != ENTITYNUM_NONE;
	}

	bool PMove::InAir()
	{
		return cgs->predictedPlayerState.groundEntityNum == ENTITYNUM_NONE;
	}
}
