#pragma once
#include "Game/Base.hpp"

#include "Game/System/Net.hpp"

namespace IW3SR
{
	// Malformed covers anything the reader could not fully trust, never a half filled A2SInfo.
	enum class A2SResult
	{
		Malformed,
		Info,
		Challenge,
		Split
	};

	// An A2S_INFO reply, plus the block CoD4X servers append after the Valve payload.
	struct A2SInfo
	{
		int Protocol = 0;
		std::string Hostname;
		std::string Map;
		std::string Folder;
		std::string Game;
		uint16_t AppId = 0;
		int Players = 0;
		int MaxPlayers = 0;
		int Bots = 0;
		char ServerType = 0;
		char Environment = 0;
		bool Password = false;
		bool Secure = false;
		std::string Version;

		uint16_t Port = 0;
		uint64_t SteamId = 0;
		uint64_t GameId = 0;
		std::string Keywords;

		bool Extended = false;
		int Challenge = 0;
		std::string GameType;
		int FriendlyFire = 0;
		bool Killcam = false;
		bool Hardcore = false;
		bool Oldschool = false;
		bool Voice = false;
	};

	struct A2SReply
	{
		netadr_t Address = {};
		A2SInfo Info;
		int Ping = 0;
	};

	// Bounds checked reader over one datagram; a read past the end fails the whole parse.
	class A2SReader
	{
	public:
		explicit A2SReader(std::span<const uint8_t> data);

		uint8_t ReadByte();
		uint16_t ReadShort();
		uint32_t ReadLong();
		uint64_t ReadLongLong();
		std::string ReadString(size_t limit = 1024);

		bool Failed() const;
		size_t Remaining() const;

	private:
		std::span<const uint8_t> Data;
		size_t Offset = 0;
		bool Error = false;

		const uint8_t* Take(size_t count);
	};

	// Standalone Valve A2S_INFO client
	class A2S
	{
	public:
		static std::vector<uint8_t> BuildInfoRequest(uint32_t challenge);
		static A2SResult Parse(std::span<const uint8_t> packet, A2SInfo& info, uint32_t& challenge);

		static bool Query(const netadr_t& address, int timeoutMs, A2SInfo& info, int& ping);
		static std::vector<A2SReply> QueryMany(std::span<const netadr_t> addresses, int timeoutMs);

		static void Apply(serverInfo_t& server, const A2SInfo& info, int ping);
	};
}
