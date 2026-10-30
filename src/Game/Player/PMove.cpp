#include "PMove.hpp"

#include "Game/Player/Movements/CS.hpp"
#include "Game/Player/Movements/Q3.hpp"
#include "Game/Player/Movements/Q3Weapons.hpp"
#include "Game/Player/Q3Missiles.hpp"
#include "Game/Renderer/VR/VR.hpp"
#include "Game/System/Protocol.hpp"
#include "Game/System/Timestep.hpp"

#define Q3_WEAPON_HISTORY 256

namespace IW3SR
{
	struct Q3WeaponFrame
	{
		int serverTime;
		Q3WeaponState state;
	};

	// Missiles in flight are not in the playerstate, and prediction replays every command from the last
	// snapshot each frame. So the state after each predicted command is kept by its time, and a step starts
	// from the state after the command the playerstate ends on.
	static std::array<Q3WeaponFrame, Q3_WEAPON_HISTORY> WeaponHistory{};
	static int WeaponHistoryIndex = 0;
	static Q3WeaponState WeaponState{};
	static Q3WeaponEvents WeaponEvents{};
	static bool WeaponStep = false;

	// The call taken out of PmoveSingle for as long as a move is only being predicted.
	constexpr uintptr_t PredictionPatch = 0x537D10;
	static int PredictionDepth = 0;

	PMove::Prediction::Prediction()
	{
		if (!PredictionDepth++)
			Memory::Write(PredictionPatch, "\xC3");
	}

	PMove::Prediction::~Prediction()
	{
		if (!--PredictionDepth)
			Memory::Write(PredictionPatch, "\x81");
	}

	void PMove::FinishMove(usercmd_s* cmd)
	{
		CL_FinishMove_h(cmd);
		Timestep::Sample(*cmd);
		GVR::FinishMove(cmd);

		if (cgs->predictedPlayerState.pm_type != PM_NORMAL)
			return;

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
			Q3::WalkMove(pm, pml, false);
			break;
		case MovementMode::Q3CPM:
			Q3::WalkMove(pm, pml, true);
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
			Q3::AirMove(pm, pml, false);
			break;
		case MovementMode::Q3CPM:
			Q3::AirMove(pm, pml, true);
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
		if (WeaponStep)
			Q3Weapons::Detonate(pm, WeaponState, IsQ3Cpm(), &WeaponEvents);

		TraceGround(pm, pml);
		Timestep::Step(pm, pml);

		EventPMoveGroundTrace event(pm, pml);
		Application::Dispatch(event);
	}

	void PMove::TraceGround(pmove_t* pm, pml_t* pml)
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
	}

	// Brackets a predicted step of the local player so the Q3 weapons run from the right missiles.
	void PMove::PmoveSingle(pmove_t* pm)
	{
		WeaponStep = pm->ps == &cgs->predictedPlayerState && HasQ3Weapons();

		if (WeaponStep)
		{
			const auto frame = std::ranges::find(WeaponHistory, pm->ps->commandTime, &Q3WeaponFrame::serverTime);
			WeaponState = frame != WeaponHistory.end() ? frame->state : Q3WeaponState{};
		}
		PmoveSingle_h(pm);

		if (WeaponStep)
		{
			const auto frame = std::ranges::find(WeaponHistory, pm->ps->commandTime, &Q3WeaponFrame::serverTime);
			if (frame != WeaponHistory.end())
				frame->state = WeaponState;
			else
			{
				WeaponHistory[WeaponHistoryIndex] = { pm->ps->commandTime, WeaponState };
				WeaponHistoryIndex = (WeaponHistoryIndex + 1) % Q3_WEAPON_HISTORY;
			}
			Q3Missiles::Events(WeaponEvents);
			WeaponEvents.clear();
		}
		WeaponStep = false;
	}

	// Runs PM_Weapon, then launches a Q3 missile when it fired a Q3 weapon, as SR-Server does.
	void PMove::Weapon(pmove_t* pm, pml_t* pml)
	{
		const int weaponTime = pm->ps->weaponTime;
		const int sequence = pm->ps->eventSequence;

		PM_Weapon(pm, pml);

		if (!WeaponStep)
			return;

		Q3Weapons::Switch(pm->ps, IsQ3Cpm());
		if (!Q3Weapons::Fired(pm->ps, sequence))
			return;

		for (int i = 0; i < static_cast<int>(Q3Weapons::Defs.size()); i++)
		{
			if (BG_FindWeaponIndexForName(Q3Weapons::Defs[i].item) != static_cast<int>(pm->ps->weapon))
				continue;

			Q3Weapons::Fire(pm, pml, WeaponState, i, weaponTime, IsQ3Cpm(), &WeaponEvents);
			return;
		}
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
		if (ps->viewHeightCurrent == 11)
			pm.maxs[2] = 30;
		else if (ps->viewHeightCurrent == 40)
			pm.maxs[2] = 50;
		else
			pm.maxs[2] = 70;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.handler = 1;

		return pm;
	}

	void PMove::PredictPmoveSingle(pmove_t* pm, int amount)
	{
		Prediction prediction;

		for (int i = 0; i < amount; i++)
			PmoveSingle(pm);
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

	const Q3WeaponState* PMove::FindWeapons(int commandTime)
	{
		const auto frame = std::ranges::find(WeaponHistory, commandTime, &Q3WeaponFrame::serverTime);
		return frame != WeaponHistory.end() ? &frame->state : nullptr;
	}

	// A Q3 mode on a server that runs its rocket and plasma in pmove.
	bool PMove::HasQ3Weapons()
	{
		const int mode = statData->stats.data.bytedata[1700]; // sr_mode: Q3, Q3CPM, Q3CPMW
		return mode >= 3 && mode <= 5 && GProtocol::SystemInfoValue("sr_q3weapons") == "1";
	}

	bool PMove::IsQ3Cpm()
	{
		const int mode = statData->stats.data.bytedata[1700]; // sr_mode
		return mode == 4 || mode == 5;
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
