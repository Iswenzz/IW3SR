#include "Rcon.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include <cctype>
#include <iomanip>

namespace IW3SR
{
	constexpr std::string_view Connectionless{ "\xff\xff\xff\xff", 4 };

	constexpr int ReplyWindow = 10000;
	constexpr int MaxReplies = 16;
	constexpr size_t MaxReply = 2048;

	static auto Quoted(std::string& value)
	{
		return std::quoted(value, '"', '\0');
	}

	static std::string_view Trim(std::string_view text)
	{
		constexpr std::string_view space = " \t\r\n";

		const size_t first = text.find_first_not_of(space);
		if (first == std::string_view::npos)
			return {};

		return text.substr(first, text.find_last_not_of(space) - first + 1);
	}

	static bool Equals(std::string_view a, std::string_view b)
	{
		return std::ranges::equal(a, b, [](char x, char y)
			{ return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
	}

	void GRcon::Initialize()
	{
		// Retail archives rcon_password, leaving it in config_mp.cfg in plain text. Adopt what is
		// already there once, then drop DVAR_SAVED so the next config write loses it for good.
		const auto password = Dvar::Find("rcon_password");
		if (!password)
			return;

		password->flags = static_cast<DvarFlags>(password->flags & ~DVAR_SAVED);

		if (password->type != DvarType::STRING || !password->current.string || !*password->current.string)
			return;

		const std::string value = password->current.string;
		if (value.size() <= MaxRconPassword)
		{
			Store(value);
			Log::WriteLine(Channel::Info, "Adopted the archived rcon_password, it is no longer written to config.");
		}
	}

	void GRcon::Shutdown()
	{
		Wipe();

		Net::Close(Socket);
		Socket = InvalidSocket;
	}

	void GRcon::Frame()
	{
		if (Socket == InvalidSocket || Net::Milliseconds() - Deadline > 0)
			return;

		Receive();
	}

	// Takes over the whole rcon family, so the password never reaches the engine's own copy.
	bool GRcon::Command(const std::string& command)
	{
		if (Patch::UseCoD4X)
			return false;

		const std::string_view line = Trim(command);
		if (!line.starts_with("rcon"))
			return false;
		if (line.size() > 4 && !std::isspace(static_cast<unsigned char>(line[4])))
			return false;

		const std::string arguments{ Trim(line.substr(4)) };

		std::istringstream stream(arguments);
		std::string verb;
		stream >> verb;

		if (verb.empty())
		{
			Print("USAGE: rcon <command> <options...>\n");
			return true;
		}
		if (Equals(verb, "login"))
		{
			std::string password;
			stream >> Quoted(password);

			Login(password);
			return true;
		}
		if (Equals(verb, "logout"))
		{
			Logout();
			return true;
		}
		if (Equals(verb, "host"))
		{
			std::string value;
			stream >> Quoted(value);

			SetHost(value);
			return true;
		}
		Send(arguments);
		return true;
	}

	bool GRcon::Login(const std::string& password)
	{
		if (password.empty())
		{
			Print("USAGE: rcon login <password>\n");
			return false;
		}
		if (password.size() > MaxRconPassword)
		{
			Print(std::format("rcon password must be {} characters or less\n", MaxRconPassword));
			return false;
		}
		Store(password);
		Print("Set the rcon login.\n");
		return true;
	}

	void GRcon::Logout()
	{
		if (!Password[0])
		{
			Print("Not logged in\n");
			return;
		}
		Wipe();
		Print("Cleared the rcon login.\n");
	}

	bool GRcon::SetHost(const std::string& value)
	{
		if (value.empty())
		{
			Print("USAGE: rcon host <address>\n");
			return false;
		}
		netadr_t address = {};
		if (!Net::Resolve(Net::ParseEndpoint(value, PortServer), address))
		{
			Error("^1Bad rcon host address.\n");
			return false;
		}
		Host = address;
		Print(std::format("Setting rcon host to {}\n", Net::ToString(Host)));
		return true;
	}

	// Out of band from a socket of our own, so the reply comes back here instead of to an engine that
	// never saw the command.
	bool GRcon::Send(const std::string& arguments)
	{
		netadr_t address = {};
		if (!Target(address))
		{
			Print("Can't determine rcon target.  You can fix this by either:\n");
			Print("1) Joining the server as a player.\n");
			Print("2) Setting the host server with 'rcon host <address>'.\n");
			return false;
		}
		if (!Password[0])
		{
			Print("You need to log in with 'rcon login <password>' before using rcon.\n");
			return false;
		}
		if (Socket == InvalidSocket)
			Socket = Net::OpenUdp();
		if (Socket == InvalidSocket)
		{
			Error("^1Could not open a socket for rcon.\n");
			return false;
		}
		const std::string_view password = Password.data();
		const bool quote = password.contains(' ');

		std::string message{ Connectionless };
		message += "rcon ";

		if (quote)
			message += '"';
		message += password;
		if (quote)
			message += '"';

		message += ' ';
		message += arguments;

		if (Net::SendTo(Socket, address, message.c_str(), static_cast<int>(message.size()) + 1) < 0)
		{
			Error("^1Failed to send the rcon packet.\n");
			return false;
		}
		Deadline = Net::Milliseconds() + ReplyWindow;
		return true;
	}

	// A server we are playing on wins over one named by 'rcon host', the order the engine picks in.
	bool GRcon::Target(netadr_t& address)
	{
		if (client_ui && client_ui->connectionState >= CA_CONNECTED && clc.netchan.remoteAddress.type == NA_IP)
		{
			address = clc.netchan.remoteAddress;
			return true;
		}
		if (Host.type == NA_IP)
		{
			address = Host;
			return true;
		}
		return false;
	}

	void GRcon::Receive()
	{
		std::array<char, MaxReply> buffer = {};

		for (int i = 0; i < MaxReplies; ++i)
		{
			netadr_t from = {};

			const int size = Net::ReceiveFrom(Socket, buffer.data(), static_cast<int>(buffer.size()), 0, from);
			if (size <= 0)
				return;

			std::string_view packet(buffer.data(), size);
			if (!packet.starts_with(Connectionless))
				continue;

			packet.remove_prefix(Connectionless.size());
			if (!packet.starts_with("print"))
				continue;

			packet.remove_prefix(5);
			if (packet.starts_with('\n'))
				packet.remove_prefix(1);

			// The payload carries its terminator, and anything after it is padding.
			if (const size_t end = packet.find('\0'); end != std::string_view::npos)
				packet = packet.substr(0, end);

			if (!packet.empty())
				Print(std::string(packet));
		}
	}

	void GRcon::Store(const std::string& password)
	{
		Wipe();
		std::memcpy(Password.data(), password.data(), std::min(password.size(), MaxRconPassword));
	}

	void GRcon::Wipe()
	{
		SecureZeroMemory(Password.data(), Password.size());
	}

	void GRcon::Print(const std::string& text)
	{
		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, text.c_str(), 0);
	}

	void GRcon::Error(const std::string& text)
	{
		Com_PrintMessage(CON_CHANNEL_ERROR, text.c_str(), 0);
	}
}
