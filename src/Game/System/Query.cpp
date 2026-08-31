#include "Query.hpp"

#include <charconv>

namespace IW3SR
{
	constexpr uint32_t HeaderSimple = 0xFFFFFFFF;
	constexpr uint32_t HeaderSplit = 0xFFFFFFFE;

	constexpr uint8_t ReplyInfo = 'I';
	constexpr uint8_t ReplyChallenge = 'A';

	constexpr uint8_t ExtraGameId = 0x01;
	constexpr uint8_t ExtraSteamId = 0x10;
	constexpr uint8_t ExtraKeywords = 0x20;
	constexpr uint8_t ExtraSourceTv = 0x40;
	constexpr uint8_t ExtraPort = 0x80;

	// A reply larger than this is not something a browser entry needs.
	constexpr size_t MaxDatagram = 8192;

	// One address a batch is still waiting on, so a reply can be timed and a challenge answered once.
	struct A2SPending
	{
		netadr_t Address = {};
		int Sent = 0;
		bool Retried = false;
	};

	template <size_t N>
	static void CopyField(char (&field)[N], std::string_view value)
	{
		const size_t length = std::min(value.size(), N - 1);

		std::memcpy(field, value.data(), length);
		std::memset(field + length, 0, N - length);
	}

	static int ParseInt(const std::string& value)
	{
		int result = 0;
		std::from_chars(value.data(), value.data() + value.size(), result);

		return result;
	}

	A2SReader::A2SReader(std::span<const uint8_t> data) : Data(data) { }

	uint8_t A2SReader::ReadByte()
	{
		const uint8_t* at = Take(1);

		return at ? *at : 0;
	}

	uint16_t A2SReader::ReadShort()
	{
		const uint8_t* at = Take(2);
		if (!at)
			return 0;

		return static_cast<uint16_t>(at[0] | (at[1] << 8));
	}

	uint32_t A2SReader::ReadLong()
	{
		const uint8_t* at = Take(4);
		if (!at)
			return 0;

		return static_cast<uint32_t>(at[0]) | (static_cast<uint32_t>(at[1]) << 8) | (static_cast<uint32_t>(at[2]) << 16)
			| (static_cast<uint32_t>(at[3]) << 24);
	}

	uint64_t A2SReader::ReadLongLong()
	{
		const uint64_t low = ReadLong();
		const uint64_t high = ReadLong();

		return low | (high << 32);
	}

	// A string that never terminates is a truncated packet, not an empty field.
	std::string A2SReader::ReadString(size_t limit)
	{
		std::string value;
		if (Error)
			return value;

		while (Offset < Data.size())
		{
			const uint8_t byte = Data[Offset++];
			if (!byte)
				return value;

			if (value.size() < limit)
				value.push_back(static_cast<char>(byte));
		}

		Error = true;
		return {};
	}

	bool A2SReader::Failed() const
	{
		return Error;
	}

	size_t A2SReader::Remaining() const
	{
		return Error ? 0 : Data.size() - Offset;
	}

	const uint8_t* A2SReader::Take(size_t count)
	{
		if (Error || Data.size() - Offset < count)
		{
			Error = true;
			return nullptr;
		}

		const uint8_t* at = Data.data() + Offset;
		Offset += count;

		return at;
	}

	// The challenge is appended only once a server has asked for one; servers that predate it ignore
	// the four extra bytes.
	std::vector<uint8_t> A2S::BuildInfoRequest(uint32_t challenge)
	{
		static constexpr std::string_view payload = "TSource Engine Query";

		std::vector<uint8_t> packet;
		packet.reserve(4 + payload.size() + 1 + sizeof(challenge));

		packet.insert(packet.end(), 4, 0xFF);
		packet.insert(packet.end(), payload.begin(), payload.end());
		packet.push_back(0);

		if (challenge)
		{
			for (int shift = 0; shift < 32; shift += 8)
				packet.push_back(static_cast<uint8_t>(challenge >> shift));
		}
		return packet;
	}

	A2SResult A2S::Parse(std::span<const uint8_t> packet, A2SInfo& info, uint32_t& challenge)
	{
		challenge = 0;

		A2SReader reader(packet);
		const uint32_t header = reader.ReadLong();

		// Not worth reassembling for a browser ping, but not corruption either.
		if (header == HeaderSplit)
			return A2SResult::Split;
		if (header != HeaderSimple)
			return A2SResult::Malformed;

		const uint8_t type = reader.ReadByte();
		if (reader.Failed())
			return A2SResult::Malformed;

		if (type == ReplyChallenge)
		{
			challenge = reader.ReadLong();
			return reader.Failed() ? A2SResult::Malformed : A2SResult::Challenge;
		}
		if (type != ReplyInfo)
			return A2SResult::Malformed;

		info = {};
		info.Protocol = reader.ReadByte();
		info.Hostname = reader.ReadString(256);
		info.Map = reader.ReadString(64);
		info.Folder = reader.ReadString(64);
		info.Game = reader.ReadString(64);
		info.AppId = reader.ReadShort();
		info.Players = reader.ReadByte();
		info.MaxPlayers = reader.ReadByte();
		info.Bots = reader.ReadByte();
		info.ServerType = static_cast<char>(reader.ReadByte());
		info.Environment = static_cast<char>(reader.ReadByte());
		info.Password = reader.ReadByte() != 0;
		info.Secure = reader.ReadByte() != 0;
		info.Version = reader.ReadString(64);

		if (reader.Failed())
			return A2SResult::Malformed;
		if (!reader.Remaining())
			return A2SResult::Info;

		const uint8_t extra = reader.ReadByte();

		if (extra & ExtraPort)
			info.Port = reader.ReadShort();
		if (extra & ExtraSteamId)
			info.SteamId = reader.ReadLongLong();
		if (extra & ExtraSourceTv)
		{
			reader.ReadShort();
			reader.ReadString(256);
		}
		if (extra & ExtraKeywords)
			info.Keywords = reader.ReadString(256);
		if (extra & ExtraGameId)
			info.GameId = reader.ReadLongLong();

		if (reader.Failed())
			return A2SResult::Malformed;
		if (!reader.Remaining())
			return A2SResult::Info;

		const std::string text = reader.ReadString(32);
		const std::string gametype = reader.ReadString(32);
		const uint8_t friendlyFire = reader.ReadByte();
		const uint8_t killcam = reader.ReadByte();
		const uint8_t hardcore = reader.ReadByte();
		const uint8_t oldschool = reader.ReadByte();
		const uint8_t voice = reader.ReadByte();

		// A truncated extension still leaves a usable browser entry, so keep the Valve fields.
		if (reader.Failed())
			return A2SResult::Info;

		info.Extended = true;
		info.Challenge = ParseInt(text);
		info.GameType = gametype;
		info.FriendlyFire = friendlyFire;
		info.Killcam = killcam != 0;
		info.Hardcore = hardcore != 0;
		info.Oldschool = oldschool != 0;
		info.Voice = voice != 0;

		return A2SResult::Info;
	}

	bool A2S::Query(const netadr_t& address, int timeoutMs, A2SInfo& info, int& ping)
	{
		ping = 0;

		const NetSocket socket = Net::OpenUdp();
		if (socket == InvalidSocket)
			return false;

		const int start = Net::Milliseconds();
		std::vector<uint8_t> buffer(MaxDatagram);

		uint32_t challenge = 0;
		bool answered = false;

		// Two rounds: the plain request, then one carrying the challenge a modern server answers with.
		for (int attempt = 0; attempt < 2 && !answered; attempt++)
		{
			const std::vector<uint8_t> request = BuildInfoRequest(challenge);
			if (Net::SendTo(socket, address, request.data(), static_cast<int>(request.size())) < 0)
				break;

			netadr_t from = {};
			const int length =
				Net::ReceiveFrom(socket, buffer.data(), static_cast<int>(buffer.size()), timeoutMs, from);
			if (length <= 0)
				break;

			const A2SResult result = Parse({ buffer.data(), static_cast<size_t>(length) }, info, challenge);
			if (result == A2SResult::Info)
				answered = true;
			else if (result != A2SResult::Challenge)
				break;
		}

		Net::Close(socket);
		ping = std::max(1, Net::Milliseconds() - start);

		return answered;
	}

	// One socket for the whole batch: every request goes out first, then replies are collected until
	// the deadline. One at a time would cost the timeout multiplied by every dead server in the list.
	std::vector<A2SReply> A2S::QueryMany(std::span<const netadr_t> addresses, int timeoutMs)
	{
		std::vector<A2SReply> replies;
		if (addresses.empty())
			return replies;

		const NetSocket socket = Net::OpenUdp();
		if (socket == InvalidSocket)
			return replies;

		std::unordered_map<uint64_t, A2SPending> pending;
		const std::vector<uint8_t> request = BuildInfoRequest(0);

		for (const netadr_t& address : addresses)
		{
			if (address.type != NA_IP)
				continue;

			Net::SendTo(socket, address, request.data(), static_cast<int>(request.size()));
			pending[Net::Key(address)] = { address, Net::Milliseconds(), false };
		}

		std::vector<uint8_t> buffer(MaxDatagram);
		const int deadline = Net::Milliseconds() + timeoutMs;

		while (!pending.empty())
		{
			const int left = deadline - Net::Milliseconds();
			if (left <= 0)
				break;

			// Sliced, so a caller shutting down never waits out the whole window.
			netadr_t from = {};
			const int length =
				Net::ReceiveFrom(socket, buffer.data(), static_cast<int>(buffer.size()), std::min(left, 250), from);
			if (length == NetFailed)
				break;
			if (length <= 0)
				continue;

			const auto entry = pending.find(Net::Key(from));
			if (entry == pending.end())
				continue;

			A2SInfo info;
			uint32_t challenge = 0;
			const A2SResult result = Parse({ buffer.data(), static_cast<size_t>(length) }, info, challenge);

			if (result == A2SResult::Challenge && !entry->second.Retried)
			{
				const std::vector<uint8_t> retry = BuildInfoRequest(challenge);

				entry->second.Retried = true;
				Net::SendTo(socket, entry->second.Address, retry.data(), static_cast<int>(retry.size()));
				continue;
			}
			if (result == A2SResult::Info)
				replies.push_back(
					{ entry->second.Address, info, std::max(1, Net::Milliseconds() - entry->second.Sent) });

			pending.erase(entry);
		}

		Net::Close(socket);
		return replies;
	}

	// pure and hardware are asserted the way CoD4X does; a Source style reply carries neither.
	void A2S::Apply(serverInfo_t& server, const A2SInfo& info, int ping)
	{
		server.clients = static_cast<char>(std::clamp(info.Players + info.Bots, 0, 127));
		server.maxClients = static_cast<char>(std::clamp(info.MaxPlayers, 0, 127));
		server.netType = 0;
		server.allowAnonymous = 0;
		server.consoleDisabled = 0;
		server.bPassword = info.Password;
		server.pure = 1;
		server.hardware = 1;
		server.punkbuster = 0;
		server.minPing = -1;
		server.maxPing = -1;
		server.ping = static_cast<short>(std::clamp(ping, 0, 999));

		if (info.Extended)
		{
			server.friendlyfire = static_cast<char>(info.FriendlyFire);
			server.killcam = static_cast<char>(info.Killcam);
			server.voice = static_cast<char>(info.Voice);
		}

		CopyField(server.hostName, info.Hostname);
		CopyField(server.mapName, info.Map);
		CopyField(server.game, info.Folder);
		CopyField(server.gameType, info.Extended ? info.GameType : info.Game);
	}
}
