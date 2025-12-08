#include "Client.hpp"

namespace IW3SR
{
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
		CG_Respawn_h(localClientNum);

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
