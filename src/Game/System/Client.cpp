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

		EventClientPredict event;
		Application::Dispatch(event);
	}
}
