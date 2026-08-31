#include "Net.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cctype>
#include <charconv>

namespace IW3SR
{
	bool Net::Startup()
	{
		if (Started)
			return true;

		// The game has already started Winsock; this only takes a reference on it.
		WSADATA data = {};
		Started = WSAStartup(MAKEWORD(2, 2), &data) == 0;

		return Started;
	}

	void Net::Shutdown()
	{
		if (!Started)
			return;

		Started = false;
		WSACleanup();
	}

	// Splits "host", "host:port" or "[v6]:port".
	NetEndpoint Net::ParseEndpoint(std::string_view value, uint16_t defaultPort)
	{
		NetEndpoint endpoint;
		endpoint.Port = defaultPort;

		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
			value.remove_prefix(1);
		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
			value.remove_suffix(1);

		if (value.empty())
			return endpoint;

		if (value.front() == '[')
		{
			const size_t end = value.find(']');
			if (end == std::string_view::npos)
				return endpoint;

			endpoint.Host = value.substr(1, end - 1);
			value.remove_prefix(end + 1);
		}
		else
		{
			const size_t colon = value.find(':');

			endpoint.Host = value.substr(0, colon);
			value = colon == std::string_view::npos ? std::string_view() : value.substr(colon);
		}

		if (value.starts_with(':'))
		{
			int port = 0;
			const auto result = std::from_chars(value.data() + 1, value.data() + value.size(), port);

			if (result.ec == std::errc() && port > 0 && port <= UINT16_MAX)
				endpoint.Port = static_cast<uint16_t>(port);
		}
		return endpoint;
	}

	// netadr_t only carries four bytes of address, so an IPv6 only host cannot be represented.
	// Numeric only. Resolve is the one that talks to DNS, and a caller reading a list of addresses
	// wants a bad entry reported rather than a lookup per line.
	bool Net::ParseAddress(std::string_view value, netadr_t& address)
	{
		address = {};
		address.type = NA_BAD;

		const NetEndpoint endpoint = ParseEndpoint(value, 0);
		if (endpoint.Host.empty())
			return false;

		in_addr ip = {};
		if (inet_pton(AF_INET, endpoint.Host.c_str(), &ip) != 1)
			return false;

		std::memcpy(address.ip, &ip, sizeof(address.ip));
		address.type = NA_IP;
		address.port = htons(endpoint.Port);
		return true;
	}

	bool Net::Resolve(const NetEndpoint& endpoint, netadr_t& address)
	{
		address = {};
		address.type = NA_BAD;

		if (endpoint.Host.empty() || !Startup())
			return false;

		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;

		addrinfo* results = nullptr;
		if (getaddrinfo(endpoint.Host.c_str(), nullptr, &hints, &results) != 0 || !results)
			return false;

		const auto* in = reinterpret_cast<const sockaddr_in*>(results->ai_addr);

		std::memcpy(address.ip, &in->sin_addr, sizeof(address.ip));
		address.type = NA_IP;
		address.port = htons(endpoint.Port);

		freeaddrinfo(results);
		return true;
	}

	std::string Net::ToString(const netadr_t& address)
	{
		const auto ip = reinterpret_cast<const uint8_t*>(address.ip);

		if (address.type != NA_IP)
			return "invalid";

		// An address that covers a whole host carries no port, and ":0" would read as one.
		if (!address.port)
			return std::format("{}.{}.{}.{}", ip[0], ip[1], ip[2], ip[3]);

		return std::format("{}.{}.{}.{}:{}", ip[0], ip[1], ip[2], ip[3], ntohs(address.port));
	}

	bool Net::Equal(const netadr_t& a, const netadr_t& b, bool port)
	{
		if (a.type != b.type || std::memcmp(a.ip, b.ip, sizeof(a.ip)) != 0)
			return false;

		return !port || a.port == b.port;
	}

	uint64_t Net::Key(const netadr_t& address)
	{
		uint32_t ip = 0;
		std::memcpy(&ip, address.ip, sizeof(ip));

		return (static_cast<uint64_t>(ip) << 16) | address.port;
	}

	NetSocket Net::OpenUdp()
	{
		if (!Startup())
			return InvalidSocket;

		const SOCKET handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == INVALID_SOCKET)
			return InvalidSocket;

		return static_cast<NetSocket>(handle);
	}

	// Connects with a deadline instead of the several seconds Winsock spends on a dead host.
	NetSocket Net::ConnectTcp(const NetEndpoint& endpoint, int timeoutMs)
	{
		netadr_t address = {};
		if (!Resolve(endpoint, address))
			return InvalidSocket;

		const SOCKET handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (handle == INVALID_SOCKET)
			return InvalidSocket;

		sockaddr_in remote = {};
		remote.sin_family = AF_INET;
		remote.sin_port = address.port;
		std::memcpy(&remote.sin_addr, address.ip, sizeof(address.ip));

		u_long nonblocking = 1;
		ioctlsocket(handle, FIONBIO, &nonblocking);

		bool connected = connect(handle, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) == 0;
		if (!connected && WSAGetLastError() == WSAEWOULDBLOCK)
		{
			int error = 0;
			int size = sizeof(error);

			connected = Wait(static_cast<NetSocket>(handle), timeoutMs, true)
				&& getsockopt(handle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &size) == 0 && error == 0;
		}
		if (!connected)
		{
			closesocket(handle);
			return InvalidSocket;
		}

		nonblocking = 0;
		ioctlsocket(handle, FIONBIO, &nonblocking);

		return static_cast<NetSocket>(handle);
	}

	void Net::Close(NetSocket socket)
	{
		if (socket != InvalidSocket)
			closesocket(static_cast<SOCKET>(socket));
	}

	int Net::Send(NetSocket socket, const void* data, int length)
	{
		if (socket == InvalidSocket)
			return NetFailed;

		const int sent = send(static_cast<SOCKET>(socket), static_cast<const char*>(data), length, 0);

		return sent == SOCKET_ERROR ? NetFailed : sent;
	}

	int Net::SendTo(NetSocket socket, const netadr_t& address, const void* data, int length)
	{
		if (socket == InvalidSocket || address.type != NA_IP)
			return NetFailed;

		sockaddr_in remote = {};
		remote.sin_family = AF_INET;
		remote.sin_port = address.port;
		std::memcpy(&remote.sin_addr, address.ip, sizeof(address.ip));

		const int sent = sendto(static_cast<SOCKET>(socket), static_cast<const char*>(data), length, 0,
			reinterpret_cast<sockaddr*>(&remote), sizeof(remote));

		return sent == SOCKET_ERROR ? NetFailed : sent;
	}

	int Net::Receive(NetSocket socket, void* data, int length, int timeoutMs)
	{
		if (!Wait(socket, timeoutMs, false))
			return NetTimeout;

		const int read = recv(static_cast<SOCKET>(socket), static_cast<char*>(data), length, 0);

		return read == SOCKET_ERROR ? NetFailed : read;
	}

	int Net::ReceiveFrom(NetSocket socket, void* data, int length, int timeoutMs, netadr_t& from)
	{
		from = {};
		from.type = NA_BAD;

		if (!Wait(socket, timeoutMs, false))
			return NetTimeout;

		sockaddr_in remote = {};
		int size = sizeof(remote);

		const int read = recvfrom(static_cast<SOCKET>(socket), static_cast<char*>(data), length, 0,
			reinterpret_cast<sockaddr*>(&remote), &size);

		if (read == SOCKET_ERROR)
			return NetFailed;

		if (remote.sin_family == AF_INET)
		{
			from.type = NA_IP;
			from.port = remote.sin_port;
			std::memcpy(from.ip, &remote.sin_addr, sizeof(from.ip));
		}
		return read;
	}

	// Counted from the first call, so the differences never come near the end of an int.
	int Net::Milliseconds()
	{
		using namespace std::chrono;

		static const steady_clock::time_point start = steady_clock::now();

		return static_cast<int>(duration_cast<milliseconds>(steady_clock::now() - start).count());
	}

	bool Net::Wait(NetSocket socket, int timeoutMs, bool write)
	{
		if (socket == InvalidSocket)
			return false;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(static_cast<SOCKET>(socket), &set);

		timeval timeout = {};
		timeout.tv_sec = timeoutMs / 1000;
		timeout.tv_usec = (timeoutMs % 1000) * 1000;

		const int ready = select(0, write ? nullptr : &set, write ? &set : nullptr, nullptr, &timeout);

		return ready > 0;
	}
}
