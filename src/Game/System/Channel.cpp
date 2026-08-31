#include "Channel.hpp"

#include "Game/System/Crypto.hpp"
#include "Game/System/Download.hpp"
#include "Game/System/Patch.hpp"
#include "Game/System/Protocol.hpp"

#include <cstring>
#include <deque>

// On protocol 21 the gamestate, the stats handshake and downloads all come down this channel rather
// than the netchan (CoD4x-Server/src/sv_client.c:1818).
// CL_PacketEvent tests the first dword against 0xFFFFFFFF for its out of band path and ReliableMarker
// is 0xFFFFFFF0, so an untagged packet falls through the detour untouched.

namespace IW3SR
{
	// Command longs at the front of a reliable message, from svc_ops_e / clc_ops_e
	// (CoD4x-Server/src/net_game_conf.h). The two directions share the numbering.
	constexpr int32_t ReliableGamestate = 1;
	constexpr int32_t ReliableDownload = 5;
	constexpr int32_t ReliableSteam = 8;
	constexpr int32_t ReliableStats = 9;
	constexpr int32_t ClientSteam = 8;

	// statData, the block mpdata is decoded into (CoD4x_Client_pub/src/client.h:17). The trailing byte
	// says whether a profile was actually loaded into it.
	constexpr uintptr_t StatsAddress = 0xCC18C90;
	constexpr int StatsSize = 8192;
	constexpr uintptr_t StatsValidAddress = StatsAddress + StatsSize;

	// Part of the wire format, not a secret: the server decrypts against the same vector
	// (CoD4x-Server/src/sv_client.c:663).
	constexpr uint8_t StatsIv[AesBlockSize] = { 0x4F, 0x11, 0x62, 0xEB, 0x44, 0x61, 0x99, 0x66, 0xA4, 0xCF, 0x41, 0x73,
		0x99, 0x12, 0x55, 0xB9 };

	// svc_download bodies taken off the stream that GDownload has not asked for yet.
	static std::deque<std::vector<uint8_t>> Downloads;

	// Steam subcommands are ASCII, so the fold does not need a locale.
	static bool Equals(std::string_view a, std::string_view b)
	{
		const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; };

		return a.size() == b.size()
			&& std::equal(a.begin(), a.end(), b.begin(), [&](char x, char y) { return lower(x) == lower(y); });
	}

	static bool SteamInstalled()
	{
		HKEY key = nullptr;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_QUERY_VALUE, &key)
			!= ERROR_SUCCESS)
			return false;

		RegCloseKey(key);
		return true;
	}

	constexpr size_t MaxQueuedDownloads = 64;

	static int TakeDownload(uint8_t* data, int size)
	{
		if (Downloads.empty() || !data || size <= 0)
			return 0;

		const std::vector<uint8_t> front = std::move(Downloads.front());
		Downloads.pop_front();

		const int length = static_cast<int>(front.size());
		if (length > size)
		{
			Log::WriteLine(Channel::Error, "Dropping a {} byte download message; the reader offered {}.", length, size);
			return 0;
		}

		std::memcpy(data, front.data(), static_cast<size_t>(length));
		return length;
	}

	void GChannel::Initialize()
	{
		Bind();
	}

	void GChannel::Shutdown()
	{
		Messages.Disconnect();
		Downloads.clear();
	}

	bool GChannel::IsEnabled()
	{
		if (Patch::UseCoD4X || !GProtocol::UsingExtended())
			return false;

		return !GProtocol::IsLegacy();
	}

	ReliableMessages& GChannel::Instance()
	{
		return Messages;
	}

	void GChannel::Bind()
	{
		if (Bound)
			return;

		ReliableTransport::SetSender(
			[](netsrc_t sock, int length, const void* data, const netadr_t* to)
			{
				if (to && NET_SendPacket)
					NET_SendPacket(sock, length, data, *to);
			});

		DownloadTransport transport;

		transport.Send = [](const uint8_t* data, int size) { return Messages.Send(data, size); };
		transport.Receive = [](uint8_t* data, int size) { return TakeDownload(data, size); };
		transport.IsSegmented = [] { return Messages.IsActive(); };
		transport.Complete = []
		{
			// Tearing the channel down here instead would take the gamestate with it.
			if (CL_DownloadsComplete)
				CL_DownloadsComplete(0);
		};

		GDownload::SetTransport(transport);
		Bound = true;
	}

	void GChannel::Connect()
	{
		Disconnect();
	}

	void GChannel::Disconnect()
	{
		Messages.Disconnect();
		Downloads.clear();
		Attempted = false;
		Opened = false;
	}

	// CoD4X opens the channel inside CL_ConnectResponse, after Netchan_Setup (cl_main.c:3764-3798).
	// The detour here runs before retail's handler, where clc.netchan is not filled in yet, so waiting
	// for the state is what puts this after the same Netchan_Setup.
	void GChannel::Setup()
	{
		if (Attempted || !client_ui || client_ui->connectionState < CA_CONNECTED)
			return;
		if (clc.netchan.remoteAddress.type == NA_BAD || clc.netchan.remoteAddress.type == NA_LOOPBACK)
			return;

		Attempted = true;

		if (!Messages.Setup(NS_CLIENT1, clc.netchan.qport, clc.netchan.remoteAddress))
		{
			Log::WriteLine(Channel::Error, "The reliable channel could not be opened.");
			return;
		}
		Opened = true;
		// The engine keeps the qport as a sign extended short, so it prints as a negative number.
		Log::WriteLine(Channel::Game, "Reliable channel open on qport {}.", static_cast<uint16_t>(clc.netchan.qport));
	}

	void GChannel::Frame()
	{
		if (!IsEnabled())
		{
			if (Messages.IsActive())
				Disconnect();
			return;
		}

		if (!Messages.IsActive())
		{
			// ReliableMessages drops the channel on an oversize message, this port's answer to CoD4X's
			// Com_Error(ERR_DROP). Retrying the setup would sit here forever with no gamestate.
			if (Opened)
			{
				Disconnect();
				Com_PrintMessage(CON_CHANNEL_ERROR, "^1The reliable channel dropped.\n", 0);
				Cmd_ExecuteSingleCommand(0, 0, "disconnect\n");
			}
			else
				Setup();
			return;
		}

		Messages.Frame(cls ? cls->realtime : 0);

		// Bounded so a peer that never runs dry cannot hold the frame.
		std::vector<uint8_t> message;
		for (int i = 0; i < 32 && Messages.Receive(message); i++)
			Dispatch(message);

		Report();
	}

	// A gamestate is many fragments where the stats handshake is one, so "bytes climbing, no message"
	// and "nothing arriving" are different faults and this is what tells them apart.
	void GChannel::Report()
	{
		const ReliableTransport& stream = Messages.Transport();
		const int received = stream.ReceiveRate().BytesTotal;
		const int now = cls ? cls->realtime : 0;

		// Only while the session is coming up; live it would be a line a second for the whole match.
		if (!client_ui || client_ui->connectionState >= CA_ACTIVE)
			return;
		if (now - LastReport < 1000 || received == LastReceived)
			return;

		LastReport = now;
		LastReceived = received;

		// state and messageAcknowledge are what the server tests before resending a gamestate:
		// SV_ExecuteClientMessage only retries while messageAcknowledge > gamestateMessageNum
		// (sv_client.c:1900), and that only advances when the netchan delivers something.
		Log::WriteLine(Channel::Game,
			"Channel: {} in, {} out, {} held, peer window {}, state {}, msgAck {}, serverId {:#x}.", received,
			stream.SendRate().BytesTotal, stream.UsedFragmentCount(), stream.PeerWindow(),
			client_ui ? static_cast<int>(client_ui->connectionState) : -1, clc.serverMessageSequence,
			Memory::Get<uint32_t>(0xC84FE4));
	}

	bool GChannel::Send(int32_t command, const uint8_t* body, int length)
	{
		if (!Messages.IsActive() || length < 0 || (length > 0 && !body))
			return false;

		std::vector<uint8_t> message(sizeof(command) + static_cast<size_t>(length));
		std::memcpy(message.data(), &command, sizeof(command));
		if (length > 0)
			std::memcpy(message.data() + sizeof(command), body, static_cast<size_t>(length));

		return Messages.Send(message.data(), static_cast<int>(message.size()));
	}

	// Port of CL_ExecuteReliableMessage (cl_main.c:3225). The body arrives without the length prefix,
	// so the command long is at the front of it.
	void GChannel::Dispatch(std::vector<uint8_t>& message)
	{
		if (message.size() < sizeof(int32_t))
			return;

		int32_t command = 0;
		std::memcpy(&command, message.data(), sizeof(command));

		uint8_t* body = message.data() + sizeof(command);
		const int length = static_cast<int>(message.size() - sizeof(command));

		// Hex too: a length that does not match the shape its command implies means the stream is being
		// split wrongly, which no reasoning about the parser will show.
		std::string hex;
		for (int i = 0; i < length && i < 64; i++)
			hex += std::format("{:02X} ", body[i]);

		Log::WriteLine(Channel::Game, "Reliable message: command {}, {} bytes: {}", command, length, hex);

		switch (command)
		{
		case ReliableGamestate:
			GProtocol::ReliableGamestate(body, length);
			break;

		case ReliableDownload:
			// A peer that sends downloads nobody asked for cannot make this grow without bound.
			if (Downloads.size() < MaxQueuedDownloads)
				Downloads.emplace_back(body, body + length);
			break;

		case ReliableStats:
			Stats(body, length);
			break;

		case ReliableSteam:
			Steam(body, length);
			break;

		default:
			break;
		}
	}

	// SV_RequestStats gates SV_SendClientGameState (sv_client.c:1727) and asks exactly once. A type 0
	// answer would release the gamestate, but it leaves cl->stats zeroed (sv_client.c:618-680) and
	// _rank.gsc then walks the challenge table against it, throws on the first undefined value and the
	// player is kicked mid connect. So the real block goes up whenever the profile has one, encrypted
	// the way CoD4X encrypts it: AES-128 CBC keyed on this session's challenge, repeated four times
	// (cl_main.c:3705).
	bool GChannel::SendStats()
	{
		if (!Memory::Get<uint8_t>(StatsValidAddress))
			return false;

		uint8_t key[AesBlockSize] = {};
		for (size_t i = 0; i < AesBlockSize; i += sizeof(clc.challenge))
			std::memcpy(key + i, &clc.challenge, sizeof(clc.challenge));

		// [byte type][long size][the block]; the size is the one the server insists on.
		std::vector<uint8_t> message(1 + sizeof(int32_t) + StatsSize);
		message[0] = 2;

		const int32_t size = StatsSize;
		std::memcpy(message.data() + 1, &size, sizeof(size));

		if (!Aes128(key).EncryptCbc(reinterpret_cast<const uint8_t*>(StatsAddress), message.data() + 1 + sizeof(size),
				StatsSize, StatsIv))
			return false;

		if (!Send(ReliableStats, message.data(), static_cast<int>(message.size())))
			return false;

		Log::WriteLine(Channel::Game, "Answered the server's stats request with {} bytes of stats.", StatsSize);
		return true;
	}

	void GChannel::Stats(const uint8_t* body, int length)
	{
		if (length < 1 || body[0] != 0)
			return;

		if (SendStats())
			return;

		// No profile loaded, so there is nothing to send but the answer itself. CoD4X writes this byte
		// and returns without sending anything (cl_main.c:3699-3702), which leaves the handshake hanging.
		const uint8_t reply = 0;

		if (Send(ReliableStats, &reply, sizeof(reply)))
			Log::WriteLine(Channel::Game, "Answered the server's stats request.");
		else
			Log::WriteLine(Channel::Error, "Could not answer the server's stats request.");
	}

	// The other gate on the gamestate: SV_SendClientGameState needs SV_ConnectSApi as well as the stats
	// reply (sv_client.c:1726-1730). The body is [int64 serverSteamId][long command][string subcommand].
	//
	// IW3SR has no Steam client, so it answers the way CoD4X answers on a machine without one - not a
	// forged ticket, and a server that insists on a real one will still refuse. Command -1 is the patch
	// status request, CoD4X's closed integrity attestation, deliberately not implemented.
	void GChannel::Steam(const uint8_t* body, int length)
	{
		NetReader reader(body, length);

		reader.ReadLong();
		reader.ReadLong();

		const int command = reader.ReadLong();
		if (reader.Overflowed || command != 0)
			return;

		// Only these two ask for a ticket; "reset" clears state and wants no answer.
		std::string subcommand;
		for (int c = reader.ReadByte(); c > 0 && !reader.Overflowed; c = reader.ReadByte())
		{
			if (c == ' ' || c == '\t')
				break;
			subcommand.push_back(static_cast<char>(c));
		}

		if (!Equals(subcommand, "waiting") && !Equals(subcommand, "renew"))
			return;

		uint8_t data[8] = {};
		NetWriter writer(data, static_cast<int>(sizeof(data)));

		writer.WriteLong(ClientSteam);
		writer.WriteByte(1);
		writer.WriteByte(SteamInstalled() ? 1 : 0);

		if (writer.Overflowed || !Messages.Send(writer.Data, writer.CurSize))
		{
			Log::WriteLine(Channel::Error, "Could not answer the server's steam request.");
			return;
		}
		Log::WriteLine(Channel::Game, "Answered the server's steam request as a client without Steam.");
	}

	// The two things CL_PacketEvent would have done before a dispatch retail does not have: check the
	// packet came from the server, and refresh the timeout clock (cl_main.c:3385-3397).
	static bool FromServer(const netadr_t& from)
	{
		const netadr_t& server = clc.netchan.remoteAddress;

		if (from.type != server.type)
			return false;
		if (from.type == NA_LOOPBACK || from.type == NA_BOT)
			return true;

		return from.port == server.port && std::equal(std::begin(from.ip), std::end(from.ip), std::begin(server.ip));
	}

	// Returns non zero to swallow the datagram. A short packet is left for the engine so a malformed
	// one cannot be mistaken for a tagged one.
	int GChannel::PacketEvent(const netadr_t* from, msg_t* msg, int time)
	{
		if (Patch::UseCoD4X || !from || !msg || !msg->data || msg->cursize < 4)
			return 0;

		int32_t marker = 0;
		std::memcpy(&marker, msg->data, sizeof(marker));

		if (marker != ReliableMarker || !FromServer(*from))
			return 0;

		// Swallowed whatever state the channel is in, rather than handing a sequence of 0xFFFFFFF0 to
		// Netchan_Process; the peer retransmits what is dropped here.
		if (!IsEnabled() || !Messages.IsActive())
			return 1;

		clc.lastPacketTime = cls ? cls->realtime : time;

		// The marker long and the qport short both belong to the caller's half of the header.
		Messages.Transport().ReceivePacket(msg->data, msg->cursize, sizeof(marker) + sizeof(uint16_t));
		return 1;
	}
}
