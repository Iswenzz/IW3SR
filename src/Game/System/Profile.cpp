#include "Profile.hpp"
#include "Patch.hpp"
#include "Dvar.hpp"

namespace IW3SR
{
	constexpr std::string_view ProfileFolder = "CallofDuty4MW";

	constexpr uint32_t HomePathDvar = 0xCB1DCC0;

	// CoD4X calls its own format "nondecoded": magic 'ice0', the digest left zeroed and the stats
	// written in the clear (LiveStorage_ProcessNondecodedStatsData, cl_main.c:5115). Retail writes
	// 'iwm0' and signs it with an MD5 over statFilePath and the stats. CoD4X reads both - it hands
	// 'iwm0' straight to retail's own decoder - so only this direction was ever missing, and retail
	// rejecting 'ice0' is the whole reason the two clients looked like they reset each other.
	constexpr uint32_t PlainMagic = 0x30656369; // 'ice0', little endian

	// A byte at stats+0x12F records the stat_version the file was written under, and the loader
	// compares it against the dvar - both clients default it to 10, so this only fires if something
	// moves it. A mismatch clears the block and opens MENU_RESETCUSTOMCLASSES: "your online stats
	// have been reset to level 1 by Infinity Ward". Nothing is migrated on the way, so a bump only
	// ever destroys. The stats have already loaded and been marked valid by this point, so taking
	// the matching branch leaves nothing half-initialised behind it.
	//   00579ad9: 74 4e   je 0x579b29   the branch taken when the versions agree
	constexpr uintptr_t StatsVersionCheck = 0x579AD9;

	// A real stats_t is sparse - a few hundred counters spread through 8192 bytes - so a block with
	// almost no zeroes in it is not stats at all. Cheap, and it is the one check that tells a good
	// file from a buffer that was never filled in.
	static bool LooksLikeStats(const saveStatData_t& data)
	{
		const auto* bytes = reinterpret_cast<const byte*>(&data.stats);
		const auto zeros = std::count(bytes, bytes + sizeof(data.stats), 0);
		return static_cast<size_t>(zeros) > sizeof(data.stats) / 2;
	}

	// Taken before the game has opened them, so each copy is the last state that survived a run.
	// Every profile and every mod directory, because which pair is active is not known this early.
	// A file that does not read as stats is skipped rather than written over a good backup - the
	// point of the backup is the launch that goes wrong, which is exactly when the source is bad.
	static void BackupStats(const std::filesystem::path& profiles)
	{
		std::error_code error;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(profiles, error))
		{
			if (entry.path().filename() != "mpdata")
				continue;

			saveStatData_t data;
			std::ifstream file(entry.path(), std::ios::binary);

			if (!file.read(reinterpret_cast<char*>(&data), sizeof(data)) || !LooksLikeStats(data))
				continue;

			std::filesystem::copy_file(entry.path(), entry.path().parent_path() / "mpdata.bak",
				std::filesystem::copy_options::overwrite_existing, error);
		}
	}

	// Kept alive for the process: the engine stores the pointer rather than copying, and frees a
	// current value only when it differs from both the latched and the reset one - so all three are
	// set to this and it never tries to free a buffer it did not allocate.
	static std::string HomePath;

	// FS_RegisterDvars runs again on every filesystem restart, and an 'ice0' file is read again on
	// every fs_game change; neither wants its one-off work repeated hundreds of times.
	static bool Applied = false;
	static bool Announced = false;

	static bool Resolve(std::filesystem::path& path)
	{
		const char* local = std::getenv("LOCALAPPDATA");
		if (!local || !local[0])
			return false;

		path = std::filesystem::path(local) / ProfileFolder;

		std::error_code error;
		std::filesystem::create_directories(path, error);
		return !error;
	}

	// Retail splits the profile directory between the two roots. Save file I/O - active.txt and
	// config_mp.cfg - goes through fs_homepath (0x55B4D0, 0x55B2D0, 0x55C0E8), but the one search path
	// entry for "players" is built from fs_basepath, and so is the profile create/exists check. In a
	// stock install both dvars name the install directory, so nothing ever noticed.
	static void UseHomePathForProfiles()
	{
		Memory::Set<uint32_t>(0x4FB021, HomePathDvar);
		Memory::Set<uint32_t>(0x4FB0BC, HomePathDvar);
		Memory::Set<uint32_t>(0x55E6F9, HomePathDvar);
	}

	// FS_ReplaceSeparators runs over both sides first, so a path written under one separator still
	// matches one written under the other.
	static bool SamePath(const char* left, const char* right)
	{
		const auto normalize = [](const char* value)
		{
			std::string out = value;
			for (char& c : out)
				c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return out;
		};
		return normalize(left) == normalize(right);
	}

	// Retail's LiveStorage_DecodeStatsData2. An 'ice0' file carries no signature by design, so
	// verifying one would always fail and take the corrupt path; CoD4X checks nothing but the
	// fs_game the stats were written under, and neither does this. Every other magic is retail's own
	// file and keeps retail's signature check untouched.
	char Profile::DecodeStats(saveStatData_t* data, const char* gamedir)
	{
		if (!data || data->magic != PlainMagic)
			return LiveStorage_DecodeStatsData_h(data, gamedir);

		data->statFilePath[sizeof(data->statFilePath) - 1] = '\0';
		const bool mine = SamePath(data->statFilePath, gamedir ? gamedir : "");
		if (mine && !std::exchange(Announced, true))
			Log::WriteLine(Channel::Game, "Reading CoD4X stats written under {}.",
				data->statFilePath[0] ? data->statFilePath : "the base game");

		return mine ? 1 : 0;
	}

	void Profile::RegisterDvars()
	{
		FS_RegisterDvars_h();

		if (Patch::UseCoD4X)
			return;

		dvar_s* homepath = Dvar::Find("fs_homepath");
		if (!homepath || homepath->type != DvarType::STRING)
		{
			Log::WriteLine(Channel::Error, "fs_homepath is not a string dvar; profiles stay in the install.");
			return;
		}
		// Cvar_RegisterString has already applied any +set from the command line, so a current value
		// that differs from the default is a deliberate choice and outranks this.
		if (homepath->current.string && homepath->reset.string
			&& std::strcmp(homepath->current.string, homepath->reset.string) != 0)
		{
			Log::WriteLine(Channel::Game, "fs_homepath was set to {}; leaving profiles there.",
				homepath->current.string);
			return;
		}
		std::filesystem::path path;
		if (!Resolve(path))
		{
			Log::WriteLine(Channel::Error, "Could not resolve %LOCALAPPDATA%; profiles stay in the install.");
			return;
		}
		HomePath = path.string();

		Dvar::OverrideString(homepath, HomePath.c_str());

		if (std::exchange(Applied, true))
			return;

		UseHomePathForProfiles();
		Memory::Set<uint8_t>(StatsVersionCheck, 0xEB);
		BackupStats(path / "players" / "profiles");

		Log::WriteLine(Channel::Game, "Profiles are in {}.", (path / "players").string());
	}
}
