#pragma once
#include "Game/Base.hpp"

#include "Game/System/Transport.hpp"

namespace IW3SR
{
	// The reliable channel an extended session runs alongside its netchan: outbound through
	// NET_SendPacket, inbound off the CL_PacketEvent detour.
	class GChannel
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void Connect();
		static void Disconnect();
		static void Frame();

		static int PacketEvent(const netadr_t* from, msg_t* msg, int time);

		static bool Send(int32_t command, const uint8_t* body, int length);
		static ReliableMessages& Instance();
		static bool IsEnabled();

	private:
		static inline ReliableMessages Messages;
		static inline bool Bound = false;
		static inline bool Attempted = false;
		static inline bool Opened = false;
		static inline int LastReport = 0;
		static inline int LastReceived = -1;

		static void Bind();
		static void Setup();
		static void Dispatch(std::vector<uint8_t>& message);
		static bool SendStats();
		static void Stats(const uint8_t* body, int length);
		static void Steam(const uint8_t* body, int length);
		static void Report();
	};
}
