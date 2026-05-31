#include "Client.hpp"

#include "Game/Renderer/Renderer.hpp"

namespace IW3SR
{
	void Client::Initialize(int localClientNum)
	{
		CL_InitCGame_h(localClientNum);
		GRenderer::UpdateMaterials();

		auto& players = Player::GetAll();
		for (int i = 0; i < players.size(); i++)
			players[i] = CreateRef<Player>(i);

		Console::Commands.clear();
		for (int i = 0; i <= dvarCount - 1; i++)
			Console::AddCommand(dvars[i]->name);
	}

	void Client::Connect()
	{
		CL_Connect_h();

		EventClientConnect event;
		Application::Dispatch(event);
	}

	void Client::Disconnect(int localClientNum)
	{
		CL_Disconnect_h(localClientNum);

		EventClientDisconnect event;
		Application::Dispatch(event);
	}

	void Client::Respawn(int localClientNum)
	{
		EventClientSpawn event;
		Application::Dispatch(event);
	}

	void Client::Predict(int localClientNum)
	{
		CG_PredictPlayerState_Internal_h(localClientNum);

		InterpolateViewForMover();

		EventClientPredict event;
		Application::Dispatch(event);
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
