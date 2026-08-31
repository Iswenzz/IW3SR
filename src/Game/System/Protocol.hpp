#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	enum class Protocol
	{
		Unknown,
		Legacy,
		Extended
	};

	struct ProtocolHandshake
	{
		int challenge = 0;
		bool extended = false;
		int version = 0;
	};

	struct ExtendedClient
	{
		char Name[33];
		char Clantag[13];
	};

	extern int ExtendedConfigDataSequence;

	// Negotiates between stock protocol 6 and 21
	class API GProtocol
	{
	public:
		static constexpr int LegacyVersion = 6;
		static constexpr int ExtendedVersion = 21;
		static constexpr int OldestExtendedVersion = 17;
		static constexpr int OldestRawOriginVersion = 18;

		static void Initialize();
		static void Shutdown();
		static void Frame();

		static void Connect();
		static void Demo(int protocol);
		static bool Inspect(const netadr_t* from, const char* packet);

		static Protocol Negotiated();
		static bool IsLegacy();
		static int Version();
		static int Advertise();
		static bool UsingExtended();
		static const ProtocolHandshake& Handshake();

		static float ReadOriginFloat(msg_t* msg);
		static void ApplySnapshotNames(void* snapshot);
		static void ServerCommand(char* command);
		static bool ParseGamestate(void* msg);
		static void ReliableGamestate(uint8_t* body, int length);
		static void ParseConfigClient(msg_t* msg);
		static void ParseGamestateHook(int localClientNum, msg_t* msg);
		static void SystemInfoChanged();
		static bool CanParse(int protocol);

		static const char* ExtendedConfigString(int index);
		static const char* ClientName(int clientNum);
		static const char* ClientClantag(int clientNum);
		static int ConfigDataSequence();

	private:
		static inline dvar_s* AllowDvar = nullptr;
		static inline dvar_s* ProtocolDvar = nullptr;
		static inline dvar_s* CoD4XDvar = nullptr;

		static inline Protocol Current = Protocol::Unknown;
		static inline ProtocolHandshake Result = {};

		static inline bool DemoSession = false;
		static inline bool DisconnectPending = false;
		static inline bool AllowedLast = true;

		static void PrepareGamestate(int localClientNum);
		static bool ReadConfigStrings(msg_t* msg);
		static bool ReadBaseline(msg_t* msg);
		static bool ReadConfigClient(msg_t* msg);
		static void ReadMapCenter();
		static void DemoGameDir(const char* systeminfo);
		static void LoadingNewMap(const char* command);

		static void Apply(Protocol protocol);
		static void Publish();
		static bool Arm(bool reader, bool live);
		static void Reset();
		static bool Mirror();
		static bool Confirm();
		static bool ChallengeResponse(const netadr_t* from, const std::vector<std::string>& args);

		static const char* ConfigString(int index);
		static std::vector<std::string> Tokenize(const char* packet);
		static bool Equals(std::string_view a, std::string_view b);
		static bool SameBaseAddress(const netadr_t& a, const netadr_t& b);
	};
}
