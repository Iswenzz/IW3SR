#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	struct DiscordMessage;

	struct DiscordJoinRequest
	{
		std::string UserId;
		std::string Username;
		int Expires = 0;
	};

	struct DiscordParty
	{
		std::string Id;
		std::string Secret;
		int Max = 0;
		int Private = 0;
		bool Access = false;
	};

	constexpr size_t DiscordMaxRequests = 3;

	class API GDiscord
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Frame();

		static bool Available();
		static bool Connected();
		static int Count();
		static const DiscordJoinRequest* Active();
		static void Respond(bool accept);

	private:
		static inline dvar_s* EnabledDvar = nullptr;
		static inline dvar_s* AppIdDvar = nullptr;
		static inline dvar_s* JoinDvar = nullptr;
		static inline dvar_s* RegisterDvar = nullptr;

		static inline std::array<DiscordJoinRequest, DiscordMaxRequests> Queue = {};
		static inline DiscordParty Party;

		static inline int Selected = -1;
		static inline int Pending = 0;
		static inline int NextUpdate = 0;
		static inline int NextRetry = 0;
		static inline connstate_t Previous = CA_DISCONNECTED;

		static inline bool Started = false;
		static inline bool Running = false;
		static inline bool Ready = false;
		static inline bool Registered = false;

		static void Dispatch(const DiscordMessage& message);
		static void Select();

		static void OnJoin(const std::string& secret);
		static void OnJoinRequest(const nlohmann::json& user);

		static void Update();
		static void EnterServer();
		static void Publish(const nlohmann::json& activity);
		static void ReportIdle();
		static void ReportConnecting();
		static void ReportDemo();
		static void ReportGame();

		static std::string ApplicationId();
		static std::string JoinSecret();
		static std::string MapName();
		static std::string GameType();
		static bool IsPrivate();

		static std::string ConfigString(int index);
		static std::string InfoValue(const std::string& info, const std::string& key);
	};
}
