#include "Master.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"
#include "Game/System/Protocol.hpp"
#include "Game/System/ServerFilter.hpp"

#include <random>
#include <ranges>
#include <span>

// Retail's only master, cod4master.activision.com, is baked into the binary.
// sr_masterservers replaces it: up to six ';' separated entries, each an optional '*', a host and an
// optional ':port'. The query is CoD4X's - TCP to PORT_MASTER, a NUL terminated
// "getservers <protocol> <keywords>", read until the master closes. The browser has no source of its
// own for this, so the Internet one is taken over: its "globalservers" is answered from here instead.

namespace IW3SR
{
	constexpr int ConnectTimeout = 5000;
	constexpr int QueryTimeout = 15000;
	constexpr int PingWindow = 6000;

	constexpr size_t MaxResponse = 256 * 1024;
	constexpr size_t MaxServers = 8192;

	constexpr size_t MaxPinged = 1024;
	constexpr size_t MaxListed = 50;

	constexpr uint8_t WireIp = 4;
	constexpr uint8_t WireIp6 = 5;

	constexpr size_t WireHeader = 4;
	constexpr int WireAddressesPerRecord = 3;

	constexpr size_t MaxPublished = 256;
	constexpr int BrowseWindow = ConnectTimeout + QueryTimeout;
	constexpr int ParserWindow = 2000;

	static std::string_view Trim(std::string_view value)
	{
		const size_t begin = value.find_first_not_of(" \t\r\n");
		if (begin == std::string_view::npos)
			return {};

		return value.substr(begin, value.find_last_not_of(" \t\r\n") - begin + 1);
	}

	std::vector<MasterServer> ParseMasterList(std::string_view value, uint16_t defaultPort)
	{
		std::vector<MasterServer> servers;

		while (!value.empty() && servers.size() < MaxMasterServers)
		{
			const size_t separator = value.find(';');
			std::string_view entry = Trim(value.substr(0, separator));

			value = separator == std::string_view::npos ? std::string_view() : value.substr(separator + 1);

			if (entry.empty())
				continue;

			const bool authoritative = entry.front() == '*';
			if (authoritative)
				entry.remove_prefix(1);

			const NetEndpoint endpoint = Net::ParseEndpoint(entry, defaultPort);
			if (endpoint.Host.empty())
				continue;

			servers.push_back({ endpoint.Host, endpoint.Port, authoritative });
		}
		return servers;
	}

	// Records opened by '\', a four byte header, then up to three (family, address,
	// port) tuples for the same server, ending at "\EOT" or "\EOF" (cl_main.c:1460).
	std::vector<netadr_t> ParseGetServersResponse(std::span<const uint8_t> data)
	{
		static constexpr std::string_view tag = "getserversResponse";

		std::vector<netadr_t> servers;

		if (data.size() < 4 + tag.size() || data[0] != 0xFF || data[1] != 0xFF || data[2] != 0xFF || data[3] != 0xFF)
			return servers;
		if (std::string_view(reinterpret_cast<const char*>(data.data()) + 4, tag.size()) != tag)
			return servers;

		size_t at = 4 + tag.size();
		while (at < data.size() && data[at] != '\\')
			at++;

		while (at < data.size() && servers.size() < MaxServers)
		{
			at++;
			if (data.size() - at < WireHeader)
				break;
			at += WireHeader;

			netadr_t picked = {};
			picked.type = NA_BAD;

			for (int i = 0; i < WireAddressesPerRecord; i++)
			{
				if (at >= data.size())
					return servers;

				const uint8_t family = data[at++];
				const size_t length = family == WireIp ? 4 : (family == WireIp6 ? 16 : 0);

				if (!length || data.size() - at < length + sizeof(uint16_t))
					return servers;

				// The record may also carry the server's IPv6 address; retail cannot dial it.
				if (family == WireIp && picked.type != NA_IP)
				{
					picked.type = NA_IP;
					std::memcpy(picked.ip, data.data() + at, sizeof(picked.ip));
					std::memcpy(&picked.port, data.data() + at + length, sizeof(picked.port));
				}
				at += length + sizeof(uint16_t);

				if (at >= data.size() || data[at] == '\\')
					break;
			}

			if (picked.type == NA_IP && picked.port)
				servers.push_back(picked);

			if (at >= data.size() || data[at] != '\\')
				break;

			if (data.size() - at >= 4 && data[at + 1] == 'E' && data[at + 2] == 'O'
				&& (data[at + 3] == 'T' || data[at + 3] == 'F'))
				break;
		}
		return servers;
	}

	void GMaster::Initialize()
	{
		ListDvar = Dvar::RegisterString("sr_masterservers", DVAR_SAVED,
			"Master servers to query, separated by ';', up to six. A leading '*' marks one authoritative",
			"cod4master.activision.com");
		PingDvar = Dvar::RegisterBool("sr_masterping", DVAR_SAVED,
			"Query every returned server for its name, map and ping", true);
		PublishDvar = Dvar::RegisterBool("sr_masterpublish", DVAR_SAVED,
			"Feed the servers a refresh finds into the in-game browser as well as the console", true);

		Reload();
	}

	// The worker only ever waits on a socket, so Aborted lets it fall out within a slice instead of
	// holding up the quit for the whole query timeout.
	void GMaster::Shutdown()
	{
		Aborted = true;

		if (Worker.joinable())
			Worker.join();

		Busy = false;
		Done = false;
	}

	// Drains a finished query on the main thread, so nothing prints from the worker.
	void GMaster::Frame()
	{
		if (ListDvar && ListDvar->current.string && Applied != ListDvar->current.string)
			Reload();

		if (!Done)
			return;

		Done = false;
		if (Worker.joinable())
			Worker.join();

		std::vector<std::string> lines;
		{
			std::lock_guard lock(Mutex);
			lines.swap(Lines);
		}

		for (const std::string& line : lines)
			Com_PrintMessage(CON_CHANNEL_CLIENT, line.c_str(), 0);

		// Ends the hold Browse() put on the menu, back at the parser's clock. Publish() below stamps a
		// fresh window when it has servers to show; one that found nothing lets the browser settle.
		if (Browsing && cls)
		{
			Feed({});
			cls->globalServerRequestTime -= ParserWindow;
		}
		Browsing = false;

		if (Listing)
			Publish();
		Busy = false;
	}

	// Rebuilt in the wire format retail's own parser reads, which is not the one CoD4X's masters speak:
	// one record per server, opened by '\', four address bytes and two port bytes, then \EOT. The
	// parser stops after 256 records, so a longer list has to go out in several packets.
	std::vector<uint8_t> BuildGetServersResponse(std::span<const MasterEntry> entries)
	{
		// Split so the trailing hex escape cannot swallow the 'g' that follows it.
		static constexpr std::string_view tag =
			"\xFF\xFF\xFF\xFF"
			"getserversResponse";

		std::vector<uint8_t> packet;
		packet.reserve(tag.size() + entries.size() * 7 + 4);
		packet.insert(packet.end(), tag.begin(), tag.end());

		for (const MasterEntry& entry : entries)
		{
			if (entry.Address.type != NA_IP || !entry.Address.port)
				continue;

			packet.push_back('\\');
			packet.insert(packet.end(), std::begin(entry.Address.ip), std::end(entry.Address.ip));

			const auto* port = reinterpret_cast<const uint8_t*>(&entry.Address.port);
			packet.insert(packet.end(), port, port + sizeof(entry.Address.port));
		}

		static constexpr std::string_view terminator = "\\EOT";
		packet.insert(packet.end(), terminator.begin(), terminator.end());
		return packet;
	}

	// Goes through retail's parser rather than its server array, so nothing depends on a struct layout
	// we cannot see. Besides taking the servers, the parser stamps globalServerRequestTime from the
	// engine's own clock, which is the only way to reach that clock from here - cls->realtime is a
	// different one, counted up per frame from zero. So an empty response is useful on its own.
	// Main thread only: CL_ServersResponsePacket touches the browser state the UI reads.
	void GMaster::Feed(std::span<const MasterEntry> entries)
	{
		std::vector<uint8_t> packet = BuildGetServersResponse(entries);

		msg_t msg = {};
		msg.data = packet.data();
		msg.maxsize = static_cast<int>(packet.size());
		msg.cursize = static_cast<int>(packet.size());
		msg.readcount = 0;

		// The parser only reads the payload, but records the list against this address.
		netadr_t from = {};
		from.type = NA_IP;

		CL_ServersResponsePacket(from, &msg);
	}

	void GMaster::Publish()
	{
		std::vector<MasterEntry> entries;
		{
			std::lock_guard lock(Mutex);
			entries = Results;
		}
		if (!PublishDvar || !PublishDvar->current.enabled || Patch::UseCoD4X || !cls)
			return;

		// The parser only ever appends, so the list is emptied first: a refresh replaces what the
		// browser shows instead of piling this master's answer on top of the last one's. An empty
		// result clears it and stops there, which is what a master that answered nothing means.
		cls->numglobalservers = 0;

		for (size_t at = 0; at < entries.size(); at += MaxPublished)
			Feed(std::span(entries.begin() + at, std::min(MaxPublished, entries.size() - at)));
	}

	bool GMaster::Command(const std::string& command)
	{
		const std::string_view line = Trim(command);

		if (line == "sr_serverlist" || line.starts_with("sr_serverlist "))
		{
			std::string keywords(Trim(line.substr(sizeof("sr_serverlist") - 1)));
			const bool demo = keywords == "demo";

			return Refresh(demo ? std::string() : keywords, demo);
		}
		if (line.starts_with("sr_serverinfo "))
			return Info(std::string(Trim(line.substr(sizeof("sr_serverinfo") - 1))));
		if (line == "globalservers" || line.starts_with("globalservers "))
			return Browse();

		return false;
	}

	// The browser's Internet source. Its refresh button issues "globalservers 0 1 full empty", which
	// would reach cod4master.activision.com; sr_masterservers answers it instead and Frame() feeds the
	// result back in. Falling through on sr_masterpublish 0 leaves retail's own query as the way out.
	bool GMaster::Browse()
	{
		if (Patch::UseCoD4X || !PublishDvar || !PublishDvar->current.enabled || Servers().empty() || !cls)
			return false;

		// LAN_WaitServerResponse holds the menu on "refreshing" until globalServerRequestTime runs
		// out, so it has to outlast the query rather than the few seconds retail allows a master over
		// UDP. Feed() stamps it at the parser's own window, which the rest is measured from.
		Feed({});
		cls->globalServerRequestTime += BrowseWindow - ParserWindow;
		Browsing = true;

		const dvar_s* fsRestrict = Dvar::Find("fs_restrict");
		const bool demo = fsRestrict && fsRestrict->type == DvarType::BOOLEAN && fsRestrict->current.enabled;

		// Retail pings the servers it was handed itself, and the browser hides any that never answer,
		// so a pass of our own would only delay the list.
		return Refresh({}, demo, false);
	}

	std::vector<MasterServer> GMaster::Servers()
	{
		std::lock_guard lock(Mutex);
		return List;
	}

	std::vector<MasterEntry> GMaster::Entries()
	{
		std::lock_guard lock(Mutex);
		return Results;
	}

	bool GMaster::IsAuthoritative(const netadr_t& address)
	{
		if (address.type != NA_IP)
			return false;

		std::lock_guard lock(Mutex);
		return std::ranges::any_of(Authorities,
			[&](const netadr_t& authority) { return Net::Key(authority) == Net::Key(address); });
	}

	bool GMaster::IsBusy()
	{
		return Busy;
	}

	bool GMaster::Refresh(const std::string& keywords, bool demo, bool query)
	{
		const std::vector<MasterServer> servers = Servers();
		if (servers.empty())
		{
			Com_PrintMessage(CON_CHANNEL_CLIENT, "^3sr_masterservers is empty.\n", 0);
			return true;
		}

		// Asking as protocol 6 gets an empty list back - the masters answer that with "\EOF" and only
		// list servers to a client claiming the extended protocol. So this is sr_extendedProtocol's
		// question, not one of its own: the list is the servers this client is willing to speak to.
		const int protocol = GProtocol::UsingExtended() ? GProtocol::ExtendedVersion : GProtocol::LegacyVersion;
		const bool ping = query && (!PingDvar || PingDvar->current.enabled);

		Start(
			[=]
			{
				// Dropped before the query rather than after it, so a master that cannot be reached or
				// answers with nothing leaves an empty list instead of the last one that worked.
				Listing = true;
				{
					std::lock_guard lock(Mutex);
					Results.clear();
				}

				MasterServer chosen;
				const NetSocket socket = Connect(servers, chosen);

				if (socket == InvalidSocket)
				{
					Print("^1Could not reach any master server.\n");
					return;
				}

				Print(std::format("Requesting servers from {}:{}...\n", chosen.Host, chosen.Port));

				const std::vector<uint8_t> response = Fetch(socket, Request(protocol, keywords, demo));
				Net::Close(socket);

				const std::vector<netadr_t> addresses = ParseGetServersResponse(response);
				if (addresses.empty())
				{
					Print("^3Invalid or empty response from the master server.\n");
					return;
				}

				// Filtered before anything is pinged, so a hidden server costs no query. Check()
				// rewrites the address in place for a redirect entry, hence the copy.
				std::vector<MasterEntry> entries;
				entries.reserve(addresses.size());
				for (netadr_t address : addresses)
				{
					if (ServerFilter::Check(address, FilterBrowser))
						continue;

					entries.push_back({ address, {}, 0, false });
				}
				if (entries.empty())
				{
					Print("^3Every server the master returned is on the filter list.\n");
					return;
				}

				if (ping && !Aborted)
				{
					// From the surviving entries, so a redirected address is never queried at its old one.
					std::vector<netadr_t> pinged;
					pinged.reserve(std::min(entries.size(), MaxPinged));

					for (const MasterEntry& entry : entries | std::views::take(MaxPinged))
						pinged.push_back(entry.Address);

					const std::span batch(pinged);

					for (const A2SReply& reply : A2S::QueryMany(batch, PingWindow))
					{
						const auto entry = std::ranges::find_if(entries, [&](const MasterEntry& candidate)
							{ return Net::Key(candidate.Address) == Net::Key(reply.Address); });

						if (entry == entries.end())
							continue;

						entry->Info = reply.Info;
						entry->Ping = reply.Ping;
						entry->Responded = true;
					}
				}

				size_t answered = 0;
				for (const MasterEntry& entry : entries)
				{
					if (!entry.Responded)
						continue;

					if (answered++ < MaxListed)
					{
						Print(std::format("{:>21}  {:>4}ms  {:>2}/{:<2}  {:<16} {}\n", Net::ToString(entry.Address),
							entry.Ping, entry.Info.Players, entry.Info.MaxPlayers, entry.Info.Map,
							entry.Info.Hostname));
					}
				}

				{
					std::lock_guard lock(Mutex);
					Results = std::move(entries);
				}
				Print(std::format("{} servers listed, {} answered.\n", addresses.size(), answered));
			});
		return true;
	}

	bool GMaster::Info(const std::string& address)
	{
		if (address.empty())
			return false;

		Start(
			[=]
			{
				Listing = false;

				netadr_t resolved = {};
				if (!Net::Resolve(Net::ParseEndpoint(address, PortServer), resolved))
				{
					Print(std::format("^1Could not resolve {}.\n", address));
					return;
				}

				A2SInfo info;
				int ping = 0;

				if (!A2S::Query(resolved, 2000, info, ping))
				{
					Print(std::format("^3No answer from {}.\n", Net::ToString(resolved)));
					return;
				}

				Print(std::format("{} - {}\n", Net::ToString(resolved), info.Hostname));
				Print(std::format("  map {}  gametype {}  players {}/{}  bots {}  ping {}ms\n", info.Map,
					info.Extended ? info.GameType : info.Game, info.Players, info.MaxPlayers, info.Bots, ping));
				Print(std::format("  protocol {}  version {}  password {}  mod {}\n", info.Protocol, info.Version,
					info.Password ? "yes" : "no", info.Folder));

				if (info.Extended)
				{
					Print(std::format("  friendlyfire {}  killcam {}  hardcore {}  oldschool {}  voice {}\n",
						info.FriendlyFire, info.Killcam, info.Hardcore, info.Oldschool, info.Voice));
				}
			});
		return true;
	}

	void GMaster::Reload()
	{
		const char* value = ListDvar && ListDvar->current.string ? ListDvar->current.string : "";

		Applied = value;

		std::lock_guard lock(Mutex);
		List = ParseMasterList(Applied, PortMaster);
	}

	void GMaster::Start(std::function<void()> job)
	{
		if (Busy.exchange(true))
		{
			Com_PrintMessage(CON_CHANNEL_CLIENT, "^3A server query is already running.\n", 0);
			return;
		}
		if (Worker.joinable())
			Worker.join();
		{
			std::lock_guard lock(Mutex);
			Lines.clear();
		}
		Worker = std::thread(
			[job = std::move(job)]
			{
				job();
				Done = true;
			});
	}

	void GMaster::Print(std::string line)
	{
		std::lock_guard lock(Mutex);
		Lines.push_back(std::move(line));
	}

	// Starts at a random master, so a list of six does not hammer the first entry every time.
	NetSocket GMaster::Connect(const std::vector<MasterServer>& servers, MasterServer& chosen)
	{
		std::random_device device;
		const size_t start = servers.empty() ? 0 : device() % servers.size();

		for (size_t i = 0; i < servers.size() && !Aborted; i++)
		{
			const MasterServer& server = servers[(start + i) % servers.size()];
			const NetSocket socket = Net::ConnectTcp({ server.Host, server.Port }, ConnectTimeout);

			if (socket == InvalidSocket)
				continue;

			chosen = server;

			if (server.Authoritative)
			{
				netadr_t address = {};
				if (Net::Resolve({ server.Host, server.Port }, address))
				{
					std::lock_guard lock(Mutex);
					Authorities.push_back(address);
				}
			}
			return socket;
		}
		return InvalidSocket;
	}

	std::vector<uint8_t> GMaster::Request(int protocol, const std::string& keywords, bool demo)
	{
		std::string query = std::format("getservers {}", protocol);

		if (!keywords.empty())
			query += " " + keywords;
		if (demo)
			query += " demo";

		std::vector<uint8_t> packet;
		packet.insert(packet.end(), 4, 0xFF);
		packet.insert(packet.end(), query.begin(), query.end());
		packet.push_back(0);

		return packet;
	}

	// The master answers on the same connection and closes it when done, so read until a zero length
	// recv or the deadline.
	std::vector<uint8_t> GMaster::Fetch(NetSocket socket, const std::vector<uint8_t>& request)
	{
		std::vector<uint8_t> response;

		for (size_t sent = 0; sent < request.size();)
		{
			const int wrote = Net::Send(socket, request.data() + sent, static_cast<int>(request.size() - sent));
			if (wrote < 0)
				return response;

			sent += wrote;
		}

		const int deadline = Net::Milliseconds() + QueryTimeout;
		std::vector<uint8_t> chunk(8192);

		while (response.size() < MaxResponse && !Aborted)
		{
			const int left = deadline - Net::Milliseconds();
			if (left <= 0)
				break;

			// Sliced so shutdown is not held, and a master pausing mid response is not read as a close.
			const int read = Net::Receive(socket, chunk.data(), static_cast<int>(chunk.size()), std::min(left, 250));
			if (read == NetTimeout)
				continue;
			if (read <= 0)
				break;

			response.insert(response.end(), chunk.begin(), chunk.begin() + read);
		}
		return response;
	}
}
