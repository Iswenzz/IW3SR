#include "NetSim.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include <ws2tcpip.h>

#include <random>

namespace IW3SR
{
	constexpr int MaxFakelag = 1023;
	constexpr int MaxDropsim = 100;
	constexpr int MaxPackets = 125;
	constexpr int PortRange = 16;

	constexpr size_t MaxDatagram = 65535;
	constexpr size_t MaxQueued = 1024;
	constexpr size_t MaxQueuedBytes = 4 * 1024 * 1024;
	constexpr int MaxDrain = 256;

	static Hook<int STDCALL(SOCKET socket, char* buffer, int length, int flags, sockaddr* from, int* fromLength)>
		RecvFrom_h(recvfrom, NetSim::Recv);

	static uint64_t Ticks()
	{
		using namespace std::chrono;

		return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	}

	// 1 to 100 inclusive, so a percentage compares exactly.
	static int Chance()
	{
		static std::mt19937 engine(std::random_device{}());
		static std::uniform_int_distribution<int> distribution(1, 100);

		return distribution(engine);
	}

	void NetSim::Initialize()
	{
		if (const auto maxpackets = Dvar::Find("cl_maxpackets"))
			maxpackets->domain.integer.max = MaxPackets;

		if (Patch::UseCoD4X)
			return;

		Fakelag = Dvar::RegisterInt("sr_net_fakelag", DVAR_NONE,
			"Artificial input lag on incoming packets, in milliseconds", 0, 0, MaxFakelag);
		Dropsim = Dvar::RegisterInt("sr_net_dropsim", DVAR_CHEATPROTECTED,
			"Percentage of incoming packets to throw away", 0, 0, MaxDropsim);

		Port = Dvar::Find("net_port");
		if (!Port)
			Log::WriteLine(Channel::Warning, "net_port is missing, sr_net_fakelag will do nothing.");

		Staging.resize(MaxDatagram);
		RecvFrom_h.Install();
	}

	void NetSim::Shutdown()
	{
		RecvFrom_h.Remove();
		Flush();
	}

	int STDCALL NetSim::Recv(SOCKET socket, char* buffer, int length, int flags, sockaddr* from, int* fromLength)
	{
		const int lag = Fakelag ? std::clamp(Fakelag->current.integer, 0, MaxFakelag) : 0;
		const int drop = Dropsim ? std::clamp(Dropsim->current.integer, 0, MaxDropsim) : 0;

		if (!lag && !drop)
		{
			// Drop what is queued rather than let it land in a burst the moment the dvar goes off.
			if (Buffered.load(std::memory_order_relaxed))
				Flush();

			return RecvFrom_h(socket, buffer, length, flags, from, fromLength);
		}
		// MSG_PEEK promises the datagram stays on the socket, which the queue cannot honour.
		if ((flags & MSG_PEEK) || !Owned(socket))
			return RecvFrom_h(socket, buffer, length, flags, from, fromLength);

		Drain(socket, flags, drop);
		return Release(socket, buffer, length, from, fromLength, lag);
	}

	// Everything else in the process, CEF and curl included, must keep reading its packets on time.
	bool NetSim::Owned(SOCKET socket)
	{
		int type = 0;
		int size = sizeof(type);

		if (getsockopt(socket, SOL_SOCKET, SO_TYPE, reinterpret_cast<char*>(&type), &size) != 0 || type != SOCK_DGRAM)
			return false;

		if (!Port)
			Port = Dvar::Find("net_port");
		if (!Port)
			return false;

		sockaddr_storage local = {};
		int localLength = sizeof(local);

		if (getsockname(socket, reinterpret_cast<sockaddr*>(&local), &localLength) != 0)
			return false;

		int port = 0;
		if (local.ss_family == AF_INET)
			port = ntohs(reinterpret_cast<const sockaddr_in&>(local).sin_port);
		else if (local.ss_family == AF_INET6)
			port = ntohs(reinterpret_cast<const sockaddr_in6&>(local).sin6_port);

		const int base = Port->current.integer;
		return port >= base && port < base + PortRange;
	}

	// Drains up front so the delay counts from when a packet arrived, not from the read that saw it.
	void NetSim::Drain(SOCKET socket, int flags, int drop)
	{
		const uint64_t now = Ticks();

		std::scoped_lock lock(Mutex);
		if (Staging.size() != MaxDatagram)
			Staging.resize(MaxDatagram);

		for (int i = 0; i < MaxDrain; ++i)
		{
			sockaddr_storage remote = {};
			int remoteLength = sizeof(remote);

			const int size = RecvFrom_h(socket, Staging.data(), static_cast<int>(Staging.size()), flags,
				reinterpret_cast<sockaddr*>(&remote), &remoteLength);
			if (size <= 0)
				break;

			if (drop && Chance() <= drop)
				continue;

			while (!Queue.empty() && (Queue.size() >= MaxQueued || Bytes + size > MaxQueuedBytes))
			{
				Bytes -= Queue.front().Data.size();
				Queue.pop_front();
			}

			DelayedPacket packet;
			packet.Arrival = now;
			packet.Socket = socket;
			packet.From = remote;
			packet.FromLength = remoteLength;
			packet.Data.assign(Staging.begin(), Staging.begin() + size);

			Bytes += packet.Data.size();
			Queue.push_back(std::move(packet));
		}
		Buffered.store(!Queue.empty(), std::memory_order_relaxed);
	}

	// With nothing due yet, reports the socket empty the way a real non blocking read would.
	int NetSim::Release(SOCKET socket, char* buffer, int length, sockaddr* from, int* fromLength, int lag)
	{
		const uint64_t now = Ticks();

		std::scoped_lock lock(Mutex);
		for (auto it = Queue.begin(); it != Queue.end(); ++it)
		{
			if (it->Socket != socket || it->Arrival + lag > now)
				continue;

			const int size = static_cast<int>(it->Data.size());
			const int copied = std::min(size, length);

			if (copied > 0)
				std::memcpy(buffer, it->Data.data(), copied);

			if (from && fromLength && *fromLength > 0)
			{
				const int wanted = std::min(*fromLength, it->FromLength);

				std::memcpy(from, &it->From, wanted);
				*fromLength = it->FromLength;
			}

			Bytes -= it->Data.size();
			Queue.erase(it);
			Buffered.store(!Queue.empty(), std::memory_order_relaxed);

			// Winsock truncates an oversized datagram and says so, and NET_GetPacket reads that
			// error to print its own oversize warning.
			if (copied < size)
			{
				WSASetLastError(WSAEMSGSIZE);
				return SOCKET_ERROR;
			}
			return copied;
		}

		WSASetLastError(WSAEWOULDBLOCK);
		return SOCKET_ERROR;
	}

	void NetSim::Flush()
	{
		std::scoped_lock lock(Mutex);

		Queue.clear();
		Bytes = 0;
		Buffered.store(false, std::memory_order_relaxed);
	}
}
