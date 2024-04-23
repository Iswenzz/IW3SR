#include "Player.hpp"

namespace IW3SR
{
	Player::Player(int index)
	{
		ent = &cg_entities[index];
		info = &cgs->bgs.clientinfo[index];
	}

	void Player::Initialize()
	{
		for (int i = 0; i < Players.size(); i++)
			Players[i] = CreateRef<Player>(i);
	}

	void Player::Connect()
	{
		CL_Connect_h();

		EventPlayerConnect event;
		Application::Get().Dispatch(event);
	}

	void Player::Disconnect(int localClientNum)
	{
		CL_Disconnect_h(localClientNum);

		EventPlayerDisconnect event;
		Application::Get().Dispatch(event);
	}

	void Player::Respawn()
	{
		CG_Respawn_h();

		EventPlayerSpawn event;
		Application::Get().Dispatch(event);
	}

	bool Player::IsSelf()
	{
		return ent->nextState.clientNum == cgs->clientNum;
	}

	bool Player::IsAlive()
	{
		return ent->isAlive;
	}

	bool Player::OnGround()
	{
		return ent->nextState.groundEntityNum != 1023;
	}

	bool Player::InAir()
	{
		return ent->nextState.groundEntityNum == 1023;
	}

	void Player::Predict(int localClientNum)
	{
		CG_PredictPlayerState_Internal_h(localClientNum);
		InterpolateViewForMover();
	}

	void Player::InterpolateViewForMover()
	{
		const centity_s* cent = &cg_entities[cgs->predictedPlayerState.groundEntityNum];
		const entityType_t eType = cent->nextState.eType;

		auto viewAngles = cgs->predictedPlayerState.viewangles;
		auto deltaAngles = cgs->predictedPlayerState.delta_angles;
		const int fromTime = cgs->snap->serverTime;
		const int toTime = cgs->time;

		if (eType == ET_SCRIPTMOVER || eType == ET_PLANE)
		{
			vec3 angles, oldAngles;
			BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);
			BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);
			vec3 delta = angles - oldAngles;

			viewAngles[0] += delta.x;
			viewAngles[1] += delta.y;
			viewAngles[2] += delta.z;

			deltaAngles[0] += delta.x;
			deltaAngles[1] += delta.y;
			deltaAngles[2] += delta.z;
		}
	}

	std::array<Ref<Player>, 64>& Player::GetAll()
	{
		return Players;
	}

	Ref<Player>& Player::Get(int index)
	{
		return Players[index];
	}

	Ref<Player>& Player::Self()
	{
		return Players[cgs->clientNum];
	}

	Player::operator bool() const
	{
		return ent;
	}
}
