#include "Discord.hpp"
#include "DiscordIPC.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include "Engine/Core/Utils/StringUtils.hpp"

#include <random>

namespace IW3SR
{
	// CoD4X's Discord application, which hosts the artwork the presence names.
	constexpr const char* DefaultApplicationId = "545420065596506112";
	constexpr const char* LargeImage = "cod4_main";

	constexpr int UpdateInterval = 30000;
	constexpr int EnterDelay = 100;
	constexpr int RetryInterval = 15000;
	constexpr int RequestLifetime = 30000;
	constexpr int ServerInfoString = 0;

	static int Realtime()
	{
		return cls ? cls->realtime : 0;
	}

	static int ToInt(const std::string& value)
	{
		int result = 0;
		const auto [_, error] = std::from_chars(value.data(), value.data() + value.size(), result);
		return error == std::errc() ? result : 0;
	}

	static std::string JsonString(const nlohmann::json& json, const std::string& key)
	{
		const auto it = json.find(key);
		return it != json.end() && it->is_string() ? it->get<std::string>() : std::string();
	}

	static nlohmann::json JsonObject(const nlohmann::json& json, const std::string& key)
	{
		const auto it = json.find(key);
		return it != json.end() && it->is_object() ? *it : nlohmann::json::object();
	}

	static int ParseHex(std::string_view text, size_t index)
	{
		const auto digit = [](char c) -> int
		{
			if (c >= '0' && c <= '9')
				return c - '0';
			if (c >= 'a' && c <= 'f')
				return c - 'a' + 10;
			if (c >= 'A' && c <= 'F')
				return c - 'A' + 10;
			return -1;
		};

		if (index + 1 >= text.size())
			return -1;

		const int high = digit(text[index]);
		const int low = digit(text[index + 1]);
		return high < 0 || low < 0 ? -1 : high * 16 + low;
	}

	// Server side names carry Q3 colour codes and control bytes.
	static std::string CleanString(const std::string& text)
	{
		std::string clean;
		clean.reserve(text.size());

		for (size_t i = 0; i < text.size(); i++)
		{
			if (text[i] == '^' && i + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[i + 1])))
			{
				i++;
				continue;
			}
			if (static_cast<unsigned char>(text[i]) >= ' ')
				clean += text[i];
		}
		return clean;
	}

	// Discord truncates a long line itself and cuts the map name off first.
	static void Shorten(std::string& gametype, std::string& map)
	{
		constexpr size_t limit = 30;
		constexpr size_t cut = 12;

		if (gametype.size() + map.size() <= limit)
			return;

		if (map.size() > 15)
			map = map.substr(0, cut) + "...";
		if (gametype.size() + map.size() > limit && gametype.size() > cut)
			gametype = gametype.substr(0, cut) + "...";
	}

	// Discord draws everyone sharing a party id as one group, so each server gets a fresh one.
	static std::string RandomPartyId()
	{
		static std::mt19937 engine(std::random_device{}());
		std::uniform_int_distribution<int> byte(0, 255);

		std::string id;
		id.reserve(32);

		for (int i = 0; i < 16; i++)
			id += std::format("{:02X}", static_cast<unsigned>(byte(engine)));
		return id;
	}

	void GDiscord::Initialize()
	{
		if (Started)
			return;
		Started = true;

		EnabledDvar = Dvar::RegisterBool("sr_discord", DVAR_SAVED, "Report what you are playing to Discord", true);
		AppIdDvar = Dvar::RegisterString("sr_discord_appid", DVAR_SAVED,
			"Discord application the presence is published as", DefaultApplicationId);
		JoinDvar = Dvar::RegisterBool("sr_discord_join", DVAR_SAVED,
			"Publish a join secret so friends get a join button. Never published on a password protected server", true);
		RegisterDvar = Dvar::RegisterBool("sr_discord_register", DVAR_SAVED,
			"Register the discord- URL protocol for this user, which is what lets Discord launch the game on a join",
			true);
	}

	void GDiscord::Shutdown()
	{
		if (DiscordIPC::IsOpen())
		{
			// An activity with no body clears the presence.
			DiscordIPC::Command("SET_ACTIVITY", { { "pid", static_cast<int>(GetCurrentProcessId()) } });
			DiscordIPC::Disconnect();
		}
		Queue = {};
		Selected = -1;
		Pending = 0;
		Ready = false;
		Previous = CA_DISCONNECTED;
	}

	void GDiscord::Frame()
	{
		if (!Started)
			Initialize();

		// CoD4X ships its own rich presence; two on the same pipe would overwrite each other.
		const bool running = !Patch::UseCoD4X && EnabledDvar && EnabledDvar->current.enabled;

		if (Running != running)
		{
			Running = running;
			if (!running)
				Shutdown();
		}
		if (!Running)
			return;

		const int now = Realtime();

		if (!DiscordIPC::IsOpen())
		{
			if (now < NextRetry)
				return;

			NextRetry = now + RetryInterval;
			Ready = false;

			const std::string id = ApplicationId();
			if (!DiscordIPC::Connect(id))
				return;

			if (!Registered && RegisterDvar && RegisterDvar->current.enabled)
				Registered = DiscordIPC::Register(id);
		}
		while (const auto message = DiscordIPC::Poll())
			Dispatch(*message);

		if (DiscordIPC::IsOpen() && Ready)
			Update();
	}

	bool GDiscord::Available()
	{
		return Running;
	}

	bool GDiscord::Connected()
	{
		return Running && Ready && DiscordIPC::IsOpen();
	}

	int GDiscord::Count()
	{
		Select();
		return Pending;
	}

	const DiscordJoinRequest* GDiscord::Active()
	{
		Select();
		return Selected < 0 ? nullptr : &Queue[Selected];
	}

	void GDiscord::Respond(bool accept)
	{
		Select();
		if (Selected < 0)
			return;

		DiscordJoinRequest& request = Queue[Selected];
		DiscordIPC::Command(accept ? "SEND_ACTIVITY_JOIN_INVITE" : "CLOSE_ACTIVITY_JOIN_REQUEST",
			{ { "user_id", request.UserId } });

		request.Expires = 0;
		Select();
	}

	void GDiscord::Dispatch(const DiscordMessage& message)
	{
		if (message.Opcode != DiscordOp::Frame)
			return;

		const std::string event = JsonString(message.Payload, "evt");
		const nlohmann::json data = JsonObject(message.Payload, "data");

		if (event == "READY")
		{
			Ready = true;
			DiscordIPC::Subscribe("ACTIVITY_JOIN");
			DiscordIPC::Subscribe("ACTIVITY_JOIN_REQUEST");

			// Whatever state the handshake interrupted still has to be reported.
			Previous = CA_DISCONNECTED;
			NextUpdate = 0;
			return;
		}
		if (event == "ACTIVITY_JOIN")
		{
			OnJoin(JsonString(data, "secret"));
			return;
		}
		if (event == "ACTIVITY_JOIN_REQUEST")
		{
			OnJoinRequest(JsonObject(data, "user"));
			return;
		}
		if (event == "ERROR")
			Log::WriteLine(Channel::Warning, "Discord refused a command: {}.", data.dump());
	}

	// The oldest live request owns the overlay; an expired one frees its slot without a sweep.
	void GDiscord::Select()
	{
		const int now = Realtime();

		Selected = -1;
		Pending = 0;

		for (int i = 0; i < static_cast<int>(DiscordMaxRequests); i++)
		{
			if (Queue[i].Expires <= now)
				continue;

			Pending++;
			if (Selected < 0 || Queue[i].Expires < Queue[Selected].Expires)
				Selected = i;
		}
	}

	// A join secret comes from whoever clicked the invite, so nothing decoded out of it reaches a
	// console command unchecked.
	void GDiscord::OnJoin(const std::string& secret)
	{
		constexpr size_t addressEnd = 20; // "DISCORD" + '4' + 8 hex of address + 4 hex of port

		if (!secret.starts_with("DISCORD4") || secret.size() < addressEnd)
		{
			Log::WriteLine(Channel::Warning, "Discord sent a join secret this client cannot read.");
			return;
		}
		int address[4] = {};
		for (int i = 0; i < 4; i++)
		{
			address[i] = ParseHex(secret, 8 + i * 2);
			if (address[i] < 0)
				return;
		}
		const int high = ParseHex(secret, 16);
		const int low = ParseHex(secret, 18);

		if (high < 0 || low < 0)
			return;

		// The port travels as the two raw bytes of the network order field, the way CoD4X writes it.
		const int port = (high << 8) | low;

		// CoD4X hosts append the server password; only one that cannot break out of the command is kept.
		std::string password;
		for (size_t i = addressEnd; i < secret.size(); i += 2)
		{
			const int byte = ParseHex(secret, i);
			if (byte < 0x21 || byte > 0x7E || byte == '"' || byte == ';' || byte == '\\' || byte == '$')
			{
				password.clear();
				break;
			}
			password += static_cast<char>(byte);
		}
		if (!password.empty())
			Cmd_ExecuteSingleCommand(0, 0, std::format("set password \"{}\"\n", password).c_str());

		Log::WriteLine(Channel::Game, "Joining {}.{}.{}.{}:{} from Discord.", address[0], address[1], address[2],
			address[3], port);

		Cmd_ExecuteSingleCommand(0, 0,
			std::format("connect {}.{}.{}.{}:{}\n", address[0], address[1], address[2], address[3], port).c_str());
	}

	void GDiscord::OnJoinRequest(const nlohmann::json& user)
	{
		const std::string id = JsonString(user, "id");
		const std::string name = CleanString(JsonString(user, "username"));

		if (id.empty())
			return;

		const int now = Realtime();

		for (DiscordJoinRequest& request : Queue)
		{
			if (request.Expires > now)
				continue;

			request.UserId = id;
			request.Username = name.empty() ? "A Discord user" : name;
			request.Expires = now + RequestLifetime;

			Select();
			Log::WriteLine(Channel::Game, "Discord join request from {}.", request.Username);
			return;
		}

		// Leaving it unanswered would keep it spinning on the sender's side forever.
		Log::WriteLine(Channel::Game, "Discord join request from {} declined, the queue is full.", name);
		DiscordIPC::Command("CLOSE_ACTIVITY_JOIN_REQUEST", { { "user_id", id } });
	}

	void GDiscord::Update()
	{
		const int now = Realtime();
		const connstate_t state = client_ui ? client_ui->connectionState : CA_DISCONNECTED;

		if (state != Previous)
		{
			Previous = state;
			NextUpdate = state == CA_ACTIVE ? now + EnterDelay : 0;

			if (state == CA_ACTIVE)
				EnterServer();
		}
		if (now < NextUpdate)
			return;

		NextUpdate = now + UpdateInterval;

		if (clc.demoplaying)
			ReportDemo();
		else if (state < CA_CONNECTING)
			ReportIdle();
		else if (state < CA_ACTIVE)
			ReportConnecting();
		else
			ReportGame();
	}

	void GDiscord::EnterServer()
	{
		const std::string info = ConfigString(ServerInfoString);

		Party.Id = RandomPartyId();
		Party.Max = std::clamp(ToInt(InfoValue(info, "sv_maxclients")), 0, 64);
		Party.Private = std::clamp(ToInt(InfoValue(info, "sv_privateClients")), 0, Party.Max);
		Party.Access = cgs && cgs->clientNum < Party.Private;
		Party.Secret = IsPrivate() ? std::string() : JoinSecret();
	}

	void GDiscord::Publish(const nlohmann::json& activity)
	{
		DiscordIPC::Command("SET_ACTIVITY",
			{ { "pid", static_cast<int>(GetCurrentProcessId()) }, { "activity", activity } });
	}

	void GDiscord::ReportIdle()
	{
		Party.Secret.clear();
		Publish({ { "state", "Looking to Play" }, { "details", "On menu" },
			{ "assets", { { "large_image", LargeImage } } } });
	}

	void GDiscord::ReportConnecting()
	{
		Party.Secret.clear();
		Publish({ { "state", "Connecting to a Server..." }, { "assets", { { "large_image", LargeImage } } } });
	}

	void GDiscord::ReportDemo()
	{
		Party.Secret.clear();
		Publish({ { "state", "Watching a Replay..." }, { "assets", { { "large_image", LargeImage } } } });
	}

	void GDiscord::ReportGame()
	{
		std::string gametype = GameType();
		std::string map = MapName();
		Shorten(gametype, map);

		std::string details = gametype.empty() ? map : std::format("{} - {}", gametype, map);
		if (details.empty())
			details = "In a Match";

		int used = 0;
		int privateUsed = 0;

		for (int i = 0; cgs && i < Party.Max; i++)
		{
			if (!cgs->bgs.clientinfo[i].infoValid)
				continue;

			used++;
			if (i < Party.Private)
				privateUsed++;
		}

		// A friend cannot drop into a private slot, so without one only public capacity is shown.
		const int slots = Party.Access ? Party.Max : Party.Max - Party.Private + privateUsed;

		nlohmann::json activity = { { "state", "Playing on a Server" }, { "details", details },
			{ "assets", { { "large_image", LargeImage }, { "large_text", map } } }, { "instance", false } };

		if (slots > 0)
		{
			activity["party"] = { { "id", Party.Id }, { "size", nlohmann::json::array({ used, slots }) } };

			// Discord only draws a join button when there is a secret.
			if (used < slots && !Party.Secret.empty())
				activity["secrets"] = { { "join", Party.Secret } };
		}
		Publish(activity);
	}

	std::string GDiscord::ApplicationId()
	{
		if (!AppIdDvar || !AppIdDvar->current.string || !AppIdDvar->current.string[0])
			return DefaultApplicationId;
		return AppIdDvar->current.string;
	}

	std::string GDiscord::JoinSecret()
	{
		if (clc.serverAddress.type != NA_IP)
			return {};

		std::string secret = "DISCORD4";
		for (int i = 0; i < 4; i++)
			secret += std::format("{:02X}", static_cast<unsigned>(static_cast<uint8_t>(clc.serverAddress.ip[i])));

		const auto port = reinterpret_cast<const uint8_t*>(&clc.serverAddress.port);
		secret += std::format("{:02X}{:02X}", static_cast<unsigned>(port[0]), static_cast<unsigned>(port[1]));
		return secret;
	}

	std::string GDiscord::MapName()
	{
		if (!clients)
			return {};

		const char* path = clients[0].mapname;
		std::string name = CleanString(std::string(path, strnlen(path, sizeof(clients[0].mapname))));

		if (const size_t slash = name.find_last_of("/\\"); slash != std::string::npos)
			name = name.substr(slash + 1);
		if (const size_t dot = name.find_last_of('.'); dot != std::string::npos)
			name = name.substr(0, dot);
		if (name.starts_with("mp_"))
			name = name.substr(3);

		if (!name.empty())
			name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
		return name;
	}

	std::string GDiscord::GameType()
	{
		std::string type = CleanString(InfoValue(ConfigString(ServerInfoString), "g_gametype"));

		std::ranges::transform(type, type.begin(),
			[](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
		return type;
	}

	bool GDiscord::IsPrivate()
	{
		if (!JoinDvar || !JoinDvar->current.enabled)
			return true;
		if (InfoValue(ConfigString(ServerInfoString), "pswrd") == "1")
			return true;

		const dvar_s* password = Dvar::Find("password");
		return password && password->current.string && password->current.string[0];
	}

	std::string GDiscord::ConfigString(int index)
	{
		constexpr int count = sizeof(gameState_t::stringOffsets) / sizeof(int);

		if (!clients || index < 0 || index >= count)
			return {};

		const gameState_t& state = clients[0].gameState;
		const int offset = state.stringOffsets[index];

		if (offset < 0 || offset >= static_cast<int>(sizeof(state.stringData)))
			return {};

		const char* data = state.stringData + offset;
		return std::string(data, strnlen(data, sizeof(state.stringData) - static_cast<size_t>(offset)));
	}

	// Backslash separated key/value pairs. The engine writes each key in the case its dvar was
	// declared in, hence the case insensitive compare.
	std::string GDiscord::InfoValue(const std::string& info, const std::string& key)
	{
		const std::string wanted = StringUtils::ToLower(key);
		size_t index = 0;

		while (index < info.size() && info[index] == '\\')
		{
			const size_t nameEnd = info.find('\\', index + 1);
			if (nameEnd == std::string::npos)
				break;

			const size_t valueEnd = info.find('\\', nameEnd + 1);
			const std::string name = info.substr(index + 1, nameEnd - index - 1);

			if (StringUtils::ToLower(name) == wanted)
				return info.substr(nameEnd + 1,
					valueEnd == std::string::npos ? std::string::npos : valueEnd - nameEnd - 1);

			if (valueEnd == std::string::npos)
				break;
			index = valueEnd;
		}
		return {};
	}
}
