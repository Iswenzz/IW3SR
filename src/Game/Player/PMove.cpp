#include "PMove.hpp"

#include "Game/Player/Movements/CS.hpp"
#include "Game/Player/Movements/Monkey.hpp"
#include "Game/Player/Movements/Q3.hpp"
#include "Game/Player/Movements/Q3Weapons.hpp"
#include "Game/Player/Movements/Racer.hpp"
#include "Game/Player/Q3Missiles.hpp"
#include "Game/Renderer/VR/VR.hpp"
#include "Game/System/Protocol.hpp"
#include "Game/System/Timestep.hpp"

#define Q3_WEAPON_HISTORY 256
#define MONKEY_HISTORY 256

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

	struct MonkeyFrame
	{
		int serverTime;
		MonkeyState state;
	};

	// The ball is not in the playerstate either, and is kept the way the missiles are. Any other player moved
	// here, as a listen server moves them, keeps a ball of its own.
	static std::array<MonkeyFrame, MONKEY_HISTORY> BallHistory{};
	static int BallHistoryIndex = 0;
	static MonkeyState Ball{};
	static std::unordered_map<const playerState_s*, MonkeyState> Balls;
	static MonkeyState* BallStep = nullptr;

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
		case MovementMode::SMB:
			if (BallStep)
				Monkey::Move(pm, pml, *BallStep);
			break;
		case MovementMode::MKW:
			Racer::WalkMove(pm, pml, statData->stats.data.bytedata[1702] == 1); // sr_size
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
		case MovementMode::SMB:
			if (BallStep)
				Monkey::Move(pm, pml, *BallStep);
			break;
		case MovementMode::MKW:
			Racer::AirMove(pm, pml, statData->stats.data.bytedata[1702] == 1); // sr_size
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
		case MovementMode::MKW:
			Q3::GroundTrace(pm, pml);
			break;
		case MovementMode::CS:
			CS::GroundTrace(pm, pml);
			break;
		case MovementMode::SMB:
			if (BallStep)
				Monkey::GroundTrace(pm, pml, *BallStep);
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
		const bool predicted = pm->ps == &cgs->predictedPlayerState;
		BallStep = nullptr;

		if (GetMovementMode() == MovementMode::SMB)
		{
			if (predicted)
			{
				const auto frame = std::ranges::find(BallHistory, pm->ps->commandTime, &MonkeyFrame::serverTime);
				Ball = frame != BallHistory.end() ? frame->state : MonkeyState{};
				BallStep = &Ball;
			}
			else
				BallStep = &Balls[pm->ps];
		}
		else if (!Balls.empty())
			Balls.clear();

		PmoveSingle_h(pm);

		if (BallStep && predicted)
		{
			const auto frame = std::ranges::find(BallHistory, pm->ps->commandTime, &MonkeyFrame::serverTime);
			if (frame != BallHistory.end())
				frame->state = Ball;
			else
			{
				BallHistory[BallHistoryIndex] = { pm->ps->commandTime, Ball };
				BallHistoryIndex = (BallHistoryIndex + 1) % MONKEY_HISTORY;
			}
		}
		BallStep = nullptr;

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

	// A minigame that moves its players its own way says so in a stat of its own, over the mode they chose.
	MovementMode PMove::GetMovementMode()
	{
		const int movement = statData->stats.data.bytedata[1701]; // sr_movement
		if (movement && GProtocol::SystemInfoValue("sr_movements") == "1")
		{
			if (movement == 1)
				return MovementMode::MKW;
			if (movement == 2)
				return MovementMode::SMB;
		}

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

	// How far a ride's middle may be from the player on it, past which it is not this frame's.
	static constexpr float RIDE_REACH = 64.0f;
	// The length the server gives the move of what follows a player.
	static constexpr int RIDE_FOREVER = 1 << 30;
	// From a ball's middle to its rider's feet, in the converter's units.
	static constexpr float RIDE_RADIUS = 35.0f;
	// What ball each player rolls in, as the frame that last read the snapshot found them.
	static int RiderTime = -1;
	static std::array<int, 64> RiderBalls;

	// The model the server stands where a player's ball is trails that player by the snapshots' delay. The
	// player's own, told by the rider it names, goes where the ball was predicted.
	void PMove::SceneModel(const DObj_s* obj, cpose_t* pose, int entnum)
	{
		if (!obj || !obj->numModels || !obj->models[0] || !obj->models[0]->name || entnum < 0
			|| entnum >= MAX_GENTITIES)
			return;

		const playerState_s& ps = cgs->predictedPlayerState;
		const entityState_s& state = cg_entities[entnum].nextState;
		if (state.eType == ET_PLAYER)
		{
			RiderPose(pose, entnum);
			return;
		}
		// Every mover names rider 0 until told otherwise; only one that follows never ends its move.
		if (state.eType != ET_SCRIPTMOVER || state.lerp.pos.trType != TR_INTERPOLATE
			|| state.lerp.pos.trDuration != RIDE_FOREVER)
			return;
		if (state.otherEntityNum == ps.clientNum)
			Predicted(obj->models[0]->name, pose);
	}

	// The player's own ball, or what stands inside it, put where the ball was predicted: false where it was not.
	bool PMove::Predicted(std::string_view model, cpose_t* pose)
	{
		const playerState_s& ps = cgs->predictedPlayerState;
		vec3 origin, angles;
		if (GetMovementMode() != MovementMode::SMB || !Monkey::Pose(Ball, origin, angles, !model.starts_with("smb_ball")))
			return false;
		// Watching someone else, or a demo, nothing here was predicted.
		if (glm::distance(origin, vec3(ps.origin)) > RIDE_REACH)
			return false;

		pose->origin = origin;
		pose->angles = angles;
		return true;
	}

	// Whoever rolls a ball stands on the bottom of it and turns with it, their own animation playing on. The
	// player's own goes where the ball was predicted; anyone else's by the ball the server moves for them.
	void PMove::RiderPose(cpose_t* pose, int rider)
	{
		const playerState_s& ps = cgs->predictedPlayerState;
		vec3 origin, angles;
		if (rider == ps.clientNum && GetMovementMode() == MovementMode::SMB && Monkey::Pose(Ball, origin, angles, true)
			&& glm::distance(origin, vec3(ps.origin)) <= RIDE_REACH)
		{
			pose->origin = origin;
			pose->angles = angles;
			return;
		}

		const snapshot_s* snap = cgs->snap;
		if (!snap || rider < 0 || rider >= static_cast<int>(RiderBalls.size()))
			return;

		// The snapshot is read once a frame for every ball in it, however many players are drawn.
		if (RiderTime != cgs->time)
		{
			RiderTime = cgs->time;
			RiderBalls.fill(-1);
			for (int n = 0; n < snap->numEntities; n++)
			{
				const entityState_s& state = snap->entities[n];
				const int of = state.otherEntityNum;
				if (state.eType != ET_SCRIPTMOVER || state.lerp.pos.trType != TR_INTERPOLATE
					|| state.lerp.pos.trDuration != RIDE_FOREVER || state.number < 0 || state.number >= MAX_GENTITIES
					|| of < 0 || of >= static_cast<int>(RiderBalls.size()))
					continue;

				const XModel* model = cgs->pEntityLastXModel[state.number];
				if (model && model->name && std::string_view(model->name).starts_with("smb_ball"))
					RiderBalls[of] = state.number;
			}
		}
		if (RiderBalls[rider] < 0)
			return;

		const cpose_t& ball = cg_entities[RiderBalls[rider]].pose;
		const vec3 turn = glm::radians(vec3(ball.angles));
		const float sp = sinf(turn.x), cp = cosf(turn.x), sy = sinf(turn.y), cy = cosf(turn.y);
		const float sr = sinf(turn.z), cr = cosf(turn.z);
		const vec3 up(cr * sp * cy + sr * sy, cr * sp * sy - sr * cy, cr * cp);
		pose->origin = vec3(ball.origin) - up * RIDE_RADIUS;
		pose->angles = ball.angles;
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
