#pragma once
#include "Game/Base.hpp"

#include "Game/System/Net.hpp"

namespace IW3SR
{
	enum class QoSTraffic
	{
		BestEffort,
		Background,
		ExcellentEffort,
		AudioVideo,
		Voice,
		Control
	};

	class GQoS
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void Connected();
		static void Disconnected();

		static void SetSocket(NetSocket socket);
		static bool Attach(NetSocket socket, const netadr_t& remote);
		static void Detach();

		static bool IsActive();

	private:
		static inline dvar_s* EnabledDvar = nullptr;
		static inline dvar_s* TrafficDvar = nullptr;

		static inline HMODULE Library = nullptr;
		static inline bool Unavailable = false;

		static inline HANDLE Handle = nullptr;
		static inline uint32_t Flow = 0;
		static inline NetSocket Socket = InvalidSocket;

		static bool Load();
		static void Release();
		static NetSocket FindGameSocket();
	};
}
