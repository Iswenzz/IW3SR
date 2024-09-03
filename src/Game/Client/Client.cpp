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
		Application::Dispatch(event);
	}

	void Client::Disconnect(int localClientNum)
	{
		CL_Disconnect_h(localClientNum);

		EventClientDisconnect event;
		Application::Dispatch(event);
	}

	void Client::Respawn()
	{
		CG_Respawn_h();

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
