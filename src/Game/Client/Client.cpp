#include "Client.hpp"

#include "Game/Player/Player.hpp"

namespace IW3SR
{
	void Client::Initialize()
	{
		auto& players = Player::GetAll();
		for (int i = 0; i < players.size(); i++)
			players[i] = CreateRef<Player>(i);
	}

	void Client::Connect()
	{
		CL_Connect_h();

		EventClientConnect event;
		Application::Get().Dispatch(event);
	}

	void Client::Disconnect(int localClientNum)
	{
		CL_Disconnect_h(localClientNum);

		EventClientDisconnect event;
		Application::Get().Dispatch(event);
	}

	void Client::Respawn()
	{
		CG_Respawn_h();

		EventClientSpawn event;
		Application::Get().Dispatch(event);
	}

	void Client::Predict(int localClientNum)
	{
		CG_PredictPlayerState_Internal_h(localClientNum);
		InterpolateViewForMover();
	}

	void Client::InterpolateViewForMover()
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
}
