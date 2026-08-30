#include "Shell.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include <ShlObj.h>

namespace IW3SR
{
	constexpr std::string_view Classes = "Software\\Classes";
	constexpr std::string_view Scheme = "cod4";
	constexpr std::string_view Extension = ".dm_1";
	constexpr std::string_view ProgID = "IW3SR.Demo.1";

	// Long enough for a quoted executable path plus its arguments.
	constexpr DWORD MaxValue = 2048;

	// Size of the password buffer CoD4X parses a URL into.
	constexpr size_t MaxPassword = 31;

	// std::quoted would treat a backslash as an escape and eat every Windows path separator.
	static auto Quoted(std::string& value)
	{
		return std::quoted(value, '"', '\0');
	}

	static char Lower(char c)
	{
		return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c;
	}

	static int Hex(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		return -1;
	}

	static bool Prefix(std::string_view text, std::string_view prefix)
	{
		if (text.size() < prefix.size())
			return false;

		for (size_t i = 0; i < prefix.size(); ++i)
		{
			if (Lower(text[i]) != Lower(prefix[i]))
				return false;
		}
		return true;
	}

	static std::string Decode(std::string_view value)
	{
		std::string decoded;
		decoded.reserve(value.size());

		for (size_t i = 0; i < value.size(); ++i)
		{
			const int high = value[i] == '%' && i + 2 < value.size() ? Hex(value[i + 1]) : -1;
			const int low = high >= 0 ? Hex(value[i + 2]) : -1;

			if (low < 0)
			{
				decoded += value[i];
				continue;
			}
			decoded += static_cast<char>(high * 16 + low);
			i += 2;
		}
		return decoded;
	}

	static bool IsAddress(char c)
	{
		const char lower = Lower(c);
		return (lower >= 'a' && lower <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' || c == ':'
			|| c == '[' || c == ']';
	}

	// A URL comes from outside, so nothing that could break out of the connect command survives.
	static bool IsSafe(char c)
	{
		return static_cast<unsigned char>(c) >= 0x20 && static_cast<unsigned char>(c) != 0x7F && c != '"' && c != ';'
			&& c != '\\';
	}

	static bool Same(const std::filesystem::path& left, const std::filesystem::path& right)
	{
		std::error_code ec;
		std::string first = std::filesystem::weakly_canonical(left, ec).generic_string();
		std::string second = std::filesystem::weakly_canonical(right, ec).generic_string();

		std::ranges::transform(first, first.begin(), Lower);
		std::ranges::transform(second, second.begin(), Lower);

		return !first.empty() && first == second;
	}

	void GShell::Initialize()
	{
		AssociateDvar = Dvar::RegisterBool("sr_shell", DVAR_SAVED,
			"Register the cod4:// protocol and the .dm_1 demo association for this user", true);
		ImportDvar = Dvar::RegisterBool("sr_shell_import", DVAR_SAVED,
			"Copy a demo opened from outside the demos folder into it before playing", true);

		if (Patch::UseCoD4X)
			return;

		if (!AssociateDvar || !AssociateDvar->current.enabled)
		{
			Unregister();
			return;
		}
		if (!Register())
			Log::WriteLine(Channel::Warning, "Could not register the cod4:// protocol under HKEY_CURRENT_USER.");
	}

	bool GShell::Command(const std::string& command)
	{
		std::istringstream stream(command);
		std::string name;
		stream >> name;

		if (name == "sr_shell_register")
		{
			if (Patch::UseCoD4X)
				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
					"CoD4X owns the cod4:// protocol and the .dm_1 association while it is loaded.\n", 0);
			else
				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
					Register() ? "Registered the cod4:// protocol and the .dm_1 demo association.\n"
							   : "^1Failed to write the registration under HKEY_CURRENT_USER.\n",
					0);
			return true;
		}
		if (name == "sr_shell_unregister")
		{
			Unregister();
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Removed the cod4:// protocol and the .dm_1 association.\n", 0);
			return true;
		}
		if (name == "sr_openurl" || (name == "openurl" && !Patch::UseCoD4X))
		{
			std::string url;
			stream >> Quoted(url);

			Connect(url);
			return true;
		}
		if (name == "sr_demo")
		{
			std::string demo;
			stream >> Quoted(demo);

			return Play(demo);
		}
		if (name == "demo" && !Patch::UseCoD4X)
		{
			std::string demo, mode;
			stream >> Quoted(demo) >> mode;

			if (mode == "fullpath")
				return Play(demo);
		}
		return false;
	}

	// HKEY_CURRENT_USER needs no elevation. Values are compared before writing so a launch that
	// changes nothing leaves the shell association cache alone.
	bool GShell::Register()
	{
		if (Patch::UseCoD4X)
			return false;

		const std::string exe = Executable();
		if (exe.size() < 5)
			return false;

		int changed = 0;
		int failed = 0;

		auto set = [&](const std::string& key, const char* name, const std::string& value)
		{
			switch (Write(key, name, value))
			{
			case RegistryWrite::Updated:
				++changed;
				break;
			case RegistryWrite::Failed:
				++failed;
				break;
			default:
				break;
			}
		};

		const std::string icon = std::format("{},0", exe);
		const std::string scheme = std::format("{}\\{}", Classes, Scheme);

		set(scheme, nullptr, "URL: CoD4 Connect Handler");
		set(scheme, "URL Protocol", "");
		set(scheme + "\\DefaultIcon", nullptr, icon);
		set(scheme + "\\shell\\open\\command", nullptr, std::format("\"{}\" +openurl \"%1\"", exe));

		// Taking .dm_1 from whoever already owns it is what makes two clients fight over it every
		// launch, so the association stops here.
		const std::string extension = std::format("{}\\{}", Classes, Extension);
		const auto owner = Read(extension, nullptr);

		if (!owner || owner->empty() || *owner == ProgID)
		{
			const std::string prog = std::format("{}\\{}", Classes, ProgID);

			set(extension, nullptr, std::string(ProgID));
			set(prog, nullptr, "Call of Duty 4 Demo");
			set(prog + "\\DefaultIcon", nullptr, icon);
			set(prog + "\\shell\\open\\command", nullptr, std::format("\"{}\" +sr_demo \"%1\"", exe));
		}

		if (changed)
			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

		return failed == 0;
	}

	void GShell::Unregister()
	{
		// CoD4X registers the same scheme; only ours has +openurl. Erasing the subtree blind would
		// break the cod4:// links it owns.
		const std::string scheme = std::format("{}\\{}", Classes, Scheme);
		const auto command = Read(scheme + "\\shell\\open\\command", nullptr);

		if (command && command->contains("+openurl"))
			Erase(scheme);

		Erase(std::format("{}\\{}", Classes, ProgID));

		// The extension key can hold records of other applications, only our claim on it goes.
		const std::string extension = std::format("{}\\{}", Classes, Extension);
		if (const auto owner = Read(extension, nullptr); owner && *owner == ProgID)
			Clear(extension, nullptr);

		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
	}

	// cod4://address:port/pw:password, the form CoD4X publishes.
	std::optional<ShellUrl> GShell::Parse(const std::string& url)
	{
		constexpr std::string_view scheme = "cod4://";
		if (!Prefix(url, scheme))
			return std::nullopt;

		const std::string_view rest = std::string_view(url).substr(scheme.size());

		std::string address(rest);
		std::string password;

		if (const size_t slash = rest.find('/'); slash != std::string_view::npos)
		{
			address = rest.substr(0, slash);

			const std::string_view tail = rest.substr(slash + 1);
			if (Prefix(tail, "pw:"))
				password = tail.substr(3);
		}

		address = Decode(address);
		password = Decode(password);

		address = address.substr(0, address.find(';'));
		password = password.substr(0, password.find_first_of(";/"));

		if (address.empty() || !std::ranges::all_of(address, IsAddress))
			return std::nullopt;

		std::erase_if(password, [](char c) { return !IsSafe(c); });
		if (password.size() > MaxPassword)
			password.resize(MaxPassword);

		return ShellUrl{ address, password };
	}

	bool GShell::Connect(const std::string& url)
	{
		const auto parsed = Parse(url);
		if (!parsed)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1Invalid URL. A CoD4 URL reads cod4://address:port/pw:password\n", 0);
			return false;
		}

		if (!parsed->password.empty())
			Cmd_ExecuteSingleCommand(0, 0, std::format("set password \"{}\"\n", parsed->password).c_str());

		Cmd_ExecuteSingleCommand(0, 0, std::format("connect \"{}\"\n", parsed->address).c_str());
		return true;
	}

	// The base engine only ever reads a demo out of the search path, so the absolute path the shell
	// hands over is played by name when it already sits in a demos folder and copied in when not.
	bool GShell::Play(const std::string& demo)
	{
		if (demo.empty())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "sr_demo <path to a .dm_1 file>\n", 0);
			return true;
		}
		if (Patch::UseCoD4X)
		{
			Cmd_ExecuteSingleCommand(0, 0, std::format("demo \"{}\" fullpath\n", demo).c_str());
			return true;
		}
		std::error_code ec;
		const auto path = std::filesystem::weakly_canonical(std::filesystem::path(demo), ec);

		if (ec || !std::filesystem::is_regular_file(path, ec))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, std::format("^1Demo not found: {}\n", demo).c_str(), 0);
			return true;
		}
		const auto folders = Demos();
		for (const auto& folder : folders)
		{
			if (Same(folder, path.parent_path()))
			{
				Cmd_ExecuteSingleCommand(0, 0, std::format("demo \"{}\"\n", path.filename().string()).c_str());
				return true;
			}
		}
		if (folders.empty() || !ImportDvar || !ImportDvar->current.enabled)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR,
				std::format("^1{} is outside the demos folder, move it there to play it.\n", path.string()).c_str(), 0);
			return true;
		}
		const auto target = folders.front() / path.filename();
		std::filesystem::create_directories(folders.front(), ec);
		std::filesystem::copy_file(path, target, std::filesystem::copy_options::overwrite_existing, ec);

		if (ec)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR,
				std::format("^1Could not copy the demo into {}\n", folders.front().string()).c_str(), 0);
			return true;
		}
		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Copied the demo into {}\n", folders.front().string()).c_str(), 0);

		Cmd_ExecuteSingleCommand(0, 0, std::format("demo \"{}\"\n", target.filename().string()).c_str());
		return true;
	}

	std::string GShell::Executable()
	{
		char path[MAX_PATH] = {};
		const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);

		if (!length || length >= MAX_PATH)
			return {};

		return path;
	}

	// Search order: the mod before the stock game, the writable tree before the installation.
	std::vector<std::filesystem::path> GShell::Demos()
	{
		std::vector<std::filesystem::path> folders;

		const auto game = Dvar::Find("fs_game");
		const std::string mod = game && game->current.string ? game->current.string : "";

		for (const char* name : { "fs_homepath", "fs_basepath", "fs_savepath" })
		{
			const auto root = Dvar::Find(name);
			if (!root || !root->current.string || !root->current.string[0])
				continue;

			for (const std::string& folder : { mod, std::string("main") })
			{
				if (folder.empty())
					continue;

				auto path = std::filesystem::path(root->current.string) / folder / "demos";
				if (std::ranges::find(folders, path) == folders.end())
					folders.push_back(std::move(path));
			}
		}
		return folders;
	}

	std::optional<std::string> GShell::Read(const std::string& key, const char* name)
	{
		HKEY handle = nullptr;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, key.c_str(), 0, KEY_QUERY_VALUE, &handle) != ERROR_SUCCESS)
			return std::nullopt;

		char value[MaxValue] = {};
		DWORD size = sizeof(value);
		DWORD type = REG_NONE;

		const LSTATUS status = RegQueryValueExA(handle, name, nullptr, &type, reinterpret_cast<BYTE*>(value), &size);
		RegCloseKey(handle);

		if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
			return std::nullopt;

		// A registry string does not have to be terminated, and the size counts the terminator when it is.
		while (size && value[size - 1] == '\0')
			--size;

		return std::string(value, size);
	}

	RegistryWrite GShell::Write(const std::string& key, const char* name, const std::string& value)
	{
		if (const auto current = Read(key, name); current && *current == value)
			return RegistryWrite::Unchanged;

		HKEY handle = nullptr;
		if (RegCreateKeyExA(HKEY_CURRENT_USER, key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
				&handle, nullptr)
			!= ERROR_SUCCESS)
			return RegistryWrite::Failed;

		const LSTATUS status = RegSetValueExA(handle, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
			static_cast<DWORD>(value.size() + 1));
		RegCloseKey(handle);

		return status == ERROR_SUCCESS ? RegistryWrite::Updated : RegistryWrite::Failed;
	}

	void GShell::Clear(const std::string& key, const char* name)
	{
		HKEY handle = nullptr;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, key.c_str(), 0, KEY_SET_VALUE, &handle) != ERROR_SUCCESS)
			return;

		RegDeleteValueA(handle, name);
		RegCloseKey(handle);
	}

	void GShell::Erase(const std::string& key)
	{
		RegDeleteTreeA(HKEY_CURRENT_USER, key.c_str());
	}
}
