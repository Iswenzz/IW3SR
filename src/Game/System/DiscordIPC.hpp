#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	enum class DiscordOp : uint32_t
	{
		Handshake = 0,
		Frame = 1,
		Close = 2,
		Ping = 3,
		Pong = 4
	};

	struct DiscordMessage
	{
		DiscordOp Opcode = DiscordOp::Frame;
		nlohmann::json Payload;
	};

	class DiscordIPC
	{
	public:
		static bool Connect(const std::string& applicationId);
		static void Disconnect();
		static bool IsOpen();

		static bool Send(DiscordOp opcode, const nlohmann::json& payload);
		static bool Command(const std::string& command, const nlohmann::json& args);
		static bool Subscribe(const std::string& event);
		static std::optional<DiscordMessage> Poll();

		static bool Register(const std::string& applicationId);

	private:
		static inline HANDLE Pipe = nullptr;
		static inline uint32_t Opcode = 0;
		static inline uint32_t Length = 0;
		static inline bool Framed = false;
		static inline uint64_t Nonce = 0;

		static bool Open(int index);
		static uint32_t Available();
		static bool Read(void* data, uint32_t size);
	};
}
