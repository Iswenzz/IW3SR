#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	using NetSocket = uintptr_t;

	constexpr NetSocket InvalidSocket = ~NetSocket(0);

	constexpr int NetTimeout = -1;
	constexpr int NetFailed = -2;

	constexpr uint16_t PortMaster = 20810;
	constexpr uint16_t PortServer = 28960;

	struct NetEndpoint
	{
		std::string Host;
		uint16_t Port = 0;
	};

	class Net
	{
	public:
		static bool Startup();
		static void Shutdown();

		static NetEndpoint ParseEndpoint(std::string_view value, uint16_t defaultPort);
		static bool ParseAddress(std::string_view value, netadr_t& address);
		static bool Resolve(const NetEndpoint& endpoint, netadr_t& address);
		static std::string ToString(const netadr_t& address);
		static bool Equal(const netadr_t& a, const netadr_t& b, bool port);
		static uint64_t Key(const netadr_t& address);

		static NetSocket OpenUdp();
		static NetSocket ConnectTcp(const NetEndpoint& endpoint, int timeoutMs);
		static void Close(NetSocket socket);

		static int Send(NetSocket socket, const void* data, int length);
		static int SendTo(NetSocket socket, const netadr_t& address, const void* data, int length);
		static int Receive(NetSocket socket, void* data, int length, int timeoutMs);
		static int ReceiveFrom(NetSocket socket, void* data, int length, int timeoutMs, netadr_t& from);

		static int Milliseconds();

	private:
		static inline bool Started = false;

		static bool Wait(NetSocket socket, int timeoutMs, bool write);
	};
}
