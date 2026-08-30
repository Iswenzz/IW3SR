#pragma once
#include "Game/Base.hpp"

#include "Game/System/Net.hpp"
#include "Game/System/Query.hpp"

namespace IW3SR
{
	// One entry of sr_masterservers. A leading '*' marks a master whose commands the client executes.
	struct MasterServer
	{
		std::string Host;
		uint16_t Port = 0;
		bool Authoritative = false;
	};

	struct MasterEntry
	{
		netadr_t Address = {};
		A2SInfo Info;
		int Ping = 0;
		bool Responded = false;
	};

	constexpr size_t MaxMasterServers = 6;

	std::vector<MasterServer> ParseMasterList(std::string_view value, uint16_t defaultPort);
	std::vector<netadr_t> ParseGetServersResponse(std::span<const uint8_t> data);

	class GMaster
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Frame();

		static bool Command(const std::string& command);

		static std::vector<MasterServer> Servers();
		static std::vector<MasterEntry> Entries();
		static bool IsAuthoritative(const netadr_t& address);
		static bool IsBusy();
		static void Publish();

		static bool Refresh(const std::string& keywords, bool demo, bool query = true);
		static bool Info(const std::string& address);

	private:
		static inline dvar_s* ListDvar = nullptr;
		static inline dvar_s* PingDvar = nullptr;
		static inline dvar_s* PublishDvar = nullptr;

		static inline std::string Applied;
		static inline std::vector<MasterServer> List;

		static inline std::thread Worker;
		static inline std::atomic<bool> Busy = false;
		static inline std::atomic<bool> Done = false;
		static inline std::atomic<bool> Aborted = false;
		static inline bool Browsing = false;
		static inline bool Listing = false;

		static inline std::mutex Mutex;
		static inline std::vector<netadr_t> Authorities;
		static inline std::vector<MasterEntry> Results;
		static inline std::vector<std::string> Lines;

		static bool Browse();
		static void Feed(std::span<const MasterEntry> entries);
		static void Reload();
		static void Start(std::function<void()> job);
		static void Print(std::string line);

		static NetSocket Connect(const std::vector<MasterServer>& servers, MasterServer& chosen);
		static std::vector<uint8_t> Request(int protocol, const std::string& keywords, bool demo);
		static std::vector<uint8_t> Fetch(NetSocket socket, const std::vector<uint8_t>& request);
	};
}
