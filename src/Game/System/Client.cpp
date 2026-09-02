#include "Client.hpp"

#include "Game/Renderer/Renderer.hpp"
#include "Game/System/Capture.hpp"
#include "Game/System/CdKey.hpp"
#include "Game/System/Channel.hpp"
#include "Game/System/Console.hpp"
#include "Game/System/Download.hpp"
#include "Game/System/Dvar.hpp"
#include "Game/System/Net.hpp"
#include "Game/System/Protocol.hpp"
#include "Game/System/QoS.hpp"
#include "Game/System/ServerFilter.hpp"
#include "Game/System/Timestep.hpp"

namespace IW3SR
{
	constexpr std::array ConsoleCommands = { "replayDemo", "sr_assets", "sr_assets_usage", "sr_capture", "sr_demo",
		"sr_download_cancel", "sr_download_status", "sr_openurl", "sr_render", "sr_serverfilter_list",
		"sr_serverfilter_refresh", "sr_serverinfo", "sr_serverlist", "sr_shell_register", "sr_shell_unregister",
		"snd_pause", "snd_stopambient", "snd_unpause" };

	void Client::Initialize(int localClientNum)
	{
		CL_InitCGame_h(localClientNum);

		Dvar::InitializeGame();
		GRenderer::UpdateMaterials();

		auto& players = Player::GetAll();
		for (int i = 0; i < players.size(); i++)
			players[i] = CreateRef<Player>(i);

		Console::Commands.clear();
		for (int i = 0; i <= dvarCount - 1; i++)
			Console::AddCommand(dvars[i]->name);

		Console::AddCommand("unset");
		for (const char* name : ConsoleCommands)
		{
			Console::AddCommand(name);
			GConsole::Register(name);
		}
	}

	void Client::Connect()
	{
		GCdKey::Protect();

		CL_Connect_h();

		if (ServerFilter::Check(clc.serverAddress, FilterConnect))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR,
				std::format("^1{} is on the server filter list.\n", Net::ToString(clc.serverAddress)).c_str(), 0);
			if (client_ui)
				client_ui->connectionState = CA_DISCONNECTED;
			return;
		}
		GProtocol::Connect();
		GChannel::Connect();
		GQoS::Connected();
		Timestep::Reset();

		EventClientConnect event;
		Application::Dispatch(event);
	}

	void Client::Disconnect(int localClientNum)
	{
		CL_Disconnect_h(localClientNum);
		Timestep::Reset();
		Capture::Disconnected();
		GQoS::Disconnected();
		GDownload::Disconnected();
		GChannel::Disconnect();

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
		if (!cgs->snap)
			return;

		const int groundEntityNum = cgs->predictedPlayerState.groundEntityNum;
		if (groundEntityNum < 0 || groundEntityNum >= ENTITYNUM_NONE)
			return;

		const centity_s* cent = &cg_entities[groundEntityNum];
		const entityType_t eType = cent->nextState.eType;

		auto& viewAngles = cgs->predictedPlayerState.viewangles;
		auto& deltaAngles = cgs->predictedPlayerState.delta_angles;
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
