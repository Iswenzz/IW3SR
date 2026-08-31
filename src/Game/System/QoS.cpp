#include "QoS.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

// qWAVE is not on every Windows edition, so it is bound by name and degrades to nothing when it is
// missing. The qos2.h types are declared rather than included: that header needs _WIN32_WINNT raised
// before <Windows.h>, which the precompiled header has already pulled in.

namespace IW3SR
{
	using QOS_FLOWID = uint32_t;

	struct QOS_VERSION
	{
		uint16_t MajorVersion;
		uint16_t MinorVersion;
	};

	constexpr DWORD QOS_NON_ADAPTIVE_FLOW = 0x00000002;

	using QOSCreateHandleFn = BOOL(WINAPI*)(QOS_VERSION*, HANDLE*);
	using QOSCloseHandleFn = BOOL(WINAPI*)(HANDLE);
	using QOSAddSocketToFlowFn = BOOL(WINAPI*)(HANDLE, SOCKET, sockaddr*, int, DWORD, QOS_FLOWID*);
	using QOSRemoveSocketFromFlowFn = BOOL(WINAPI*)(HANDLE, SOCKET, QOS_FLOWID, DWORD);

	static QOSCreateHandleFn QOSCreate = nullptr;
	static QOSCloseHandleFn QOSClose = nullptr;
	static QOSAddSocketToFlowFn QOSAddSocket = nullptr;
	static QOSRemoveSocketFromFlowFn QOSRemoveSocket = nullptr;

	// Winsock hands out small handles, and the game opens its socket while it is starting.
	constexpr uintptr_t MaxSocketHandle = 0x2000;

	// NET_OpenIP walks upwards from net_port until a port is free.
	constexpr uint16_t PortRange = 16;

	// The engine shallow-copies an enum dvar's domain and keeps the pointer, so a temporary vector
	// would leave the saved dvar dangling for Com_WriteConfiguration to read on quit.
	static const std::vector<const char*> TrafficNames = { "besteffort", "background", "excellenteffort", "audiovideo",
		"voice", "control" };

	void GQoS::Initialize()
	{
		EnabledDvar = Dvar::RegisterBool("sr_qos", DVAR_SAVED,
			"Ask Windows to prioritise the game socket over other traffic on this machine", true);
		TrafficDvar = Dvar::RegisterEnum("sr_qos_traffic", DVAR_SAVED, "How the game's traffic is described to qWAVE",
			static_cast<int>(QoSTraffic::Control), TrafficNames);
	}

	void GQoS::Shutdown()
	{
		Detach();

		if (Library)
			FreeLibrary(Library);

		Library = nullptr;
		QOSCreate = nullptr;
		QOSClose = nullptr;
		QOSAddSocket = nullptr;
		QOSRemoveSocket = nullptr;
	}

	void GQoS::Connected()
	{
		if (Patch::UseCoD4X || !EnabledDvar || !EnabledDvar->current.enabled)
			return;

		if (Socket == InvalidSocket)
			Socket = FindGameSocket();

		if (Socket != InvalidSocket)
			Attach(Socket, clc.serverAddress);
	}

	void GQoS::Disconnected()
	{
		Detach();
	}

	void GQoS::SetSocket(NetSocket socket)
	{
		Socket = socket;
	}

	bool GQoS::Attach(NetSocket socket, const netadr_t& remote)
	{
		if (socket == InvalidSocket || remote.type != NA_IP || !Load())
			return false;

		Detach();

		QOS_VERSION version = { 1, 0 };
		if (!QOSCreate(&version, &Handle))
		{
			Log::WriteLine(Channel::Warning, "QOSCreateHandle failed with {}.", GetLastError());
			Handle = nullptr;
			return false;
		}

		sockaddr_in address = {};
		address.sin_family = AF_INET;
		address.sin_port = remote.port;
		std::memcpy(&address.sin_addr, remote.ip, sizeof(remote.ip));

		const int traffic = TrafficDvar ? TrafficDvar->current.integer : static_cast<int>(QoSTraffic::Control);

		if (!QOSAddSocket(Handle, static_cast<SOCKET>(socket), reinterpret_cast<sockaddr*>(&address), traffic,
				QOS_NON_ADAPTIVE_FLOW, &Flow))
		{
			Log::WriteLine(Channel::Warning, "QOSAddSocketToFlow failed with {}.", GetLastError());
			Release();
			return false;
		}

		Socket = socket;
		return true;
	}

	void GQoS::Detach()
	{
		if (Handle && Flow && Socket != InvalidSocket && QOSRemoveSocket)
			QOSRemoveSocket(Handle, static_cast<SOCKET>(Socket), Flow, 0);

		Release();
	}

	bool GQoS::IsActive()
	{
		return Handle && Flow;
	}

	bool GQoS::Load()
	{
		if (QOSAddSocket)
			return true;
		if (Unavailable)
			return false;

		// LoadLibraryEx keeps this out of the LoadLibraryA detour, and pins the search to System32
		// so a qwave.dll dropped next to the game is never picked up.
		Library = LoadLibraryExA("qwave.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!Library)
		{
			Unavailable = true;
			Log::WriteLine(Channel::Debug, "qwave.dll is not available, QoS stays off.");
			return false;
		}

		QOSCreate = reinterpret_cast<QOSCreateHandleFn>(GetProcAddress(Library, "QOSCreateHandle"));
		QOSClose = reinterpret_cast<QOSCloseHandleFn>(GetProcAddress(Library, "QOSCloseHandle"));
		QOSAddSocket = reinterpret_cast<QOSAddSocketToFlowFn>(GetProcAddress(Library, "QOSAddSocketToFlow"));
		QOSRemoveSocket =
			reinterpret_cast<QOSRemoveSocketFromFlowFn>(GetProcAddress(Library, "QOSRemoveSocketFromFlow"));

		if (QOSCreate && QOSClose && QOSAddSocket && QOSRemoveSocket)
			return true;

		Unavailable = true;
		QOSCreate = nullptr;
		QOSClose = nullptr;
		QOSAddSocket = nullptr;
		QOSRemoveSocket = nullptr;

		FreeLibrary(Library);
		Library = nullptr;

		Log::WriteLine(Channel::Debug, "qwave.dll is missing the QOS2 exports, QoS stays off.");
		return false;
	}

	void GQoS::Release()
	{
		if (Handle && QOSClose)
			QOSClose(Handle);

		Handle = nullptr;
		Flow = 0;
	}

	// Retail keeps its UDP socket in a static this client cannot name, so the handles are probed
	// instead; getsockname just fails on one that is not a socket. Prefer SetSocket() when known.
	NetSocket GQoS::FindGameSocket()
	{
		const dvar_s* port = Dvar::Find("net_port");
		const uint16_t base =
			port && port->current.integer > 0 ? static_cast<uint16_t>(port->current.integer) : PortServer;

		for (uintptr_t handle = 4; handle < MaxSocketHandle; handle += 4)
		{
			sockaddr_in local = {};
			int length = sizeof(local);

			if (getsockname(static_cast<SOCKET>(handle), reinterpret_cast<sockaddr*>(&local), &length) != 0)
				continue;
			if (local.sin_family != AF_INET)
				continue;

			int type = 0;
			int size = sizeof(type);

			if (getsockopt(static_cast<SOCKET>(handle), SOL_SOCKET, SO_TYPE, reinterpret_cast<char*>(&type), &size)
				!= 0)
				continue;
			if (type != SOCK_DGRAM)
				continue;

			const uint16_t bound = ntohs(local.sin_port);
			if (bound >= base && bound < base + PortRange)
				return static_cast<NetSocket>(handle);
		}
		Log::WriteLine(Channel::Debug, "No game socket bound near port {}, QoS stays off.", base);
		return InvalidSocket;
	}
}
