#pragma once
#include "Game/Base.hpp"

#include <winsock2.h>

#include <deque>

namespace IW3SR
{
	struct DelayedPacket
	{
		uint64_t Arrival = 0;
		SOCKET Socket = INVALID_SOCKET;
		sockaddr_storage From = {};
		int FromLength = 0;
		std::vector<char> Data;
	};

	// Artificial latency and packet loss on the game's UDP sockets.
	class NetSim
	{
	public:
		static void Initialize();
		static void Shutdown();

		static int STDCALL Recv(SOCKET socket, char* buffer, int length, int flags, sockaddr* from, int* fromLength);

	private:
		static inline dvar_s* Fakelag = nullptr;
		static inline dvar_s* Dropsim = nullptr;
		static inline dvar_s* Port = nullptr;

		static inline std::deque<DelayedPacket> Queue;
		static inline std::vector<char> Staging;
		static inline std::mutex Mutex;
		static inline size_t Bytes = 0;
		static inline std::atomic<bool> Buffered = false;

		static bool Owned(SOCKET socket);
		static void Drain(SOCKET socket, int flags, int drop);
		static int Release(SOCKET socket, char* buffer, int length, sockaddr* from, int* fromLength, int lag);
		static void Flush();
	};
}
