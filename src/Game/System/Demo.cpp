#include "Demo.hpp"

#include "Engine/Core/Utils/StringUtils.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"
#include "Game/System/Protocol.hpp"

namespace IW3SR
{
	// Demo records open on a one byte type. Retail knows 0 (message data) and 1 (client archive);
	// a newer client writes 2 first to announce the protocol it recorded on.
	constexpr uint8_t MessageRecord = 0;
	constexpr uint8_t ArchiveRecord = 1;
	constexpr uint8_t ProtocolRecord = 2;

	// What the protocol record carries behind its type byte: the protocol, the offset legacy playback
	// stops at, and eight reserved bytes (CL_ReadDemoProtocolInfo, cl_main.c:7828).
	constexpr int ProtocolRecordSize = 16;

	// A protocol is one byte on the connect path, so anything larger is not a protocol number and
	// never reaches the signed comparisons downstream.
	constexpr uint32_t MaxProtocol = 255;

	constexpr std::string_view DemoExtension = ".dm_1";

	// The operand FS_IsUnrestrictedFile compares an extension against, and the extension it is meant
	// to name once its caller has stripped the leading dot.
	constexpr uintptr_t UnrestrictedOperand = 0x55B7BC;
	constexpr char UnrestrictedDemo[] = "dm_1";

	static_assert(DemoExtension.substr(1) == std::string_view(UnrestrictedDemo),
		"The whitelist entry is the demo extension without its dot.");

	void Demo::Initialize()
	{
		LastDemo = Dvar::RegisterString("sr_lastdemo", DVAR_READONLY, "The last demo played", "");

		Remember(Name);
		Unrestrict();
	}

	void Demo::Tick()
	{
		const bool playing = clc.demoplaying != 0;

		// A demo started from the command line or by double clicking never comes through the console.
		if (playing && !Playing && Name.empty() && clc.demoName[0])
			Remember(clc.demoName);

		if (!playing && Playing && !Name.empty())
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Demo finished. Type replayDemo to play {} again.\n", Name).c_str(), 0);

		Playing = playing;
	}

	// Retail answers a record type it does not know by returning without consuming that record's body,
	// so the protocol record a newer client opens the file with leaves the reader one byte in and every
	// record after it is read out of the header's own bytes: one bogus message, then "Demo file was
	// truncated". The dispatch is otherwise retail's, byte for byte (0x4690C0).
	// Only this one dispatch needs the record. It is the first in the file, and CL_PlayDemo's priming
	// loop comes through here long before the copy inlined into the per frame loop (0x45C5DC) reads
	// anything.
	void Demo::ReadMessage(int localClientNum)
	{
		if (Patch::UseCoD4X)
		{
			CL_ReadDemoMessage_h(localClientNum);
			return;
		}
		if (!clc.demofile)
		{
			CL_DemoCompleted();
			return;
		}
		uint8_t record = 0;
		if (FS_Read(&record, sizeof(record), clc.demofile) != sizeof(record))
		{
			CL_DemoCompleted();
			return;
		}
		switch (record)
		{
		case MessageRecord:
			CL_ReadDemoData(localClientNum);
			break;

		case ArchiveRecord:
			CL_ReadDemoArchive();
			break;

		case ProtocolRecord:
		{
			uint8_t header[ProtocolRecordSize];
			FS_Read(header, sizeof(header), clc.demofile);
			break;
		}
		}
	}

	bool Demo::Command(const std::string& command)
	{
		std::istringstream stream(command);
		std::string verb;
		stream >> verb;
		verb = StringUtils::ToLower(verb);

		if (verb == "replaydemo")
		{
			if (Patch::UseCoD4X)
				return false;

			Replay();
			return true;
		}
		if (verb != "demo" && verb != "timedemo")
			return false;

		std::string argument, keyword;
		stream >> std::quoted(argument, '"', '\0') >> keyword;

		if (argument.empty())
			return false;

		const bool fullpath = StringUtils::ToLower(keyword) == "fullpath";
		Remember(argument);

		// Swallowing the command keeps an unplayable demo from dropping the client.
		return !Playable(argument, fullpath);
	}

	bool Demo::Replay()
	{
		if (Name.empty())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "A demo has not been played yet.\n", 0);
			return false;
		}
		// A drive letter means a full path, which the engine only takes with the keyword. Same test
		// as CL_ReplayDemo_f (cl_main.c:7648).
		const std::string command = Name.find(':') != std::string::npos ? std::format("demo \"{}\" fullpath\n", Name)
																		: std::format("demo \"{}\"\n", Name);
		Cmd_ExecuteSingleCommand(0, 0, command.c_str());
		return true;
	}

	DemoHeader Demo::Inspect(const std::filesystem::path& path)
	{
		DemoHeader header;

		std::ifstream file(path, std::ios::binary);
		if (!file)
			return header;

		uint8_t record = 0;
		file.read(reinterpret_cast<char*>(&record), sizeof(record));

		if (!file)
			return header;

		header.Valid = true;

		if (record != ProtocolRecord)
			return header;

		uint32_t protocol = 0;
		file.read(reinterpret_cast<char*>(&protocol), sizeof(protocol));

		if (!file)
			return header;

		header.Legacy = false;
		header.Protocol = protocol;
		return header;
	}

	std::filesystem::path Demo::Resolve(const std::string& name, bool fullpath)
	{
		std::filesystem::path file = name;
		if (file.extension().string() != DemoExtension)
			file += DemoExtension;

		std::error_code ec;
		if (fullpath || file.is_absolute())
			return std::filesystem::exists(file, ec) ? file : std::filesystem::path{};

		std::vector<std::string> games;
		if (const auto game = Dvar::Find("fs_game"); game && game->current.string && game->current.string[0])
			games.emplace_back(game->current.string);
		games.emplace_back("main");

		for (const auto& base : Bases())
		{
			for (const auto& game : games)
			{
				const auto candidate = base / game / "demos" / file;
				if (std::filesystem::exists(candidate, ec))
					return candidate;
			}
			const auto relative = base / file;
			if (std::filesystem::exists(relative, ec))
				return relative;
		}
		return {};
	}

	const std::string& Demo::LastPlayed()
	{
		return Name;
	}

	void Demo::Remember(const std::string& value)
	{
		if (value.empty())
			return;

		Name = value;

		if (!LastDemo)
			return;

		Dvar::OverrideString(LastDemo, Name.c_str());
	}

	// An extended demo is read in this process by GProtocol, so the only thing left to refuse is a
	// protocol nothing here understands.
	bool Demo::Playable(const std::string& name, bool fullpath)
	{
		if (Patch::UseCoD4X)
			return true;

		const auto path = Resolve(name, fullpath);
		if (path.empty())
			return true;

		const DemoHeader header = Inspect(path);

		// Retail 1.7 stops at record types 0 and 1. Reference: CL_ReadDemoProtocolInfo, cl_main.c:7828.
		if (!header.Valid || header.Legacy)
			return true;

		if (header.Protocol <= MaxProtocol && GProtocol::CanParse(static_cast<int>(header.Protocol)))
		{
			GProtocol::Demo(static_cast<int>(header.Protocol));
			return true;
		}
		Com_PrintMessage(CON_CHANNEL_ERROR,
			std::format("^1{} was recorded on protocol {}, which this client cannot read. CoD4X plays it "
						"with bin/cod4x_{:03}.\n",
				path.filename().string(), header.Protocol, header.Protocol)
				.c_str(),
			0);
		return false;
	}

	// A search path rooted at main refuses every loose file whose extension FS_IsUnrestrictedFile does
	// not name (0x55B770, reached from FS_FOpenFileReadForThread at 0x55BBAA), which is what prints
	// "must be in an IWD or not in the main directory". Its demo entry is dead in retail: the literal
	// is an unexpanded ".dm_NETWORK_PROTOCOL_VERSION" and it still carries the dot the caller strips
	// before comparing, so no demo has ever matched and main/demos is unreadable. Nothing else reads
	// that operand, so pointing it at the extension the caller actually sees is the whole fix. CoD4X
	// reaches the same place by retrying the open through FS_SV_FOpenFileRead (cl_main.c:7563), which
	// no retail call site does.
	void Demo::Unrestrict()
	{
		if (Patch::UseCoD4X)
			return;

		Memory::Set<uintptr_t>(UnrestrictedOperand, reinterpret_cast<uintptr_t>(UnrestrictedDemo));
	}

	std::vector<std::filesystem::path> Demo::Bases()
	{
		std::vector<std::filesystem::path> bases;

		// fs_savepath only to still find demos recorded before per-user files moved out of it; the
		// engine writes new ones under the install, as CoD4X does.
		for (const char* name : { "fs_basepath", "fs_homepath", "fs_savepath" })
		{
			const auto dvar = Dvar::Find(name);
			if (dvar && dvar->current.string && dvar->current.string[0])
				bases.emplace_back(dvar->current.string);
		}

		char program[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, program, MAX_PATH);
		bases.push_back(std::filesystem::path(program).parent_path());

		return bases;
	}
}
