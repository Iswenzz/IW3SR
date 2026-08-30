#include "Profile.hpp"
#include "Patch.hpp"
#include "Dvar.hpp"

namespace IW3SR
{
	constexpr std::string_view ProfileFolder = "CallofDuty4MW";

	constexpr uint32_t HomePathDvar = 0xCB1DCC0;

	// CoD4X's stats format: plaintext and unsigned (cl_main.c:5115), unlike retail's sealed 'iwm0'.
	constexpr uint32_t PlainMagic = 0x30656369; // 'ice0'

	constexpr uintptr_t EncodeStatsData = 0x5794E0;
	constexpr uintptr_t SysTimeGetTime = 0x69136C;

	constexpr int32_t MagicField = offsetof(saveStatData_t, magic);
	constexpr int32_t SaveTimeField = offsetof(saveStatData_t, saveTime);
	constexpr int32_t DigestField = offsetof(saveStatData_t, digest);
	constexpr int32_t DigestSize = sizeof(saveStatData_t::digest);

	// stats+0x12F holds the stat_version the file was written under, and a mismatch wipes the block
	// and opens MENU_RESETCUSTOMCLASSES. Nothing is migrated on the way, so a bump only ever destroys.
	constexpr uintptr_t StatsVersionCheck = 0x579AD9;

	// A real stats_t is sparse, so a block without many zeroes was never filled in.
	static bool LooksLikeStats(const saveStatData_t& data)
	{
		const auto* bytes = reinterpret_cast<const byte*>(&data.stats);
		const auto zeros = std::count(bytes, bytes + sizeof(data.stats), 0);
		return static_cast<size_t>(zeros) > sizeof(data.stats) / 2;
	}

	// Every profile and mod directory, because which pair is active is not known this early. A file
	// that does not read as stats is skipped: a bad source is what a backup must not overwrite.
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

	// The engine stores this pointer rather than copying, and all three dvar slots point at it so it
	// never tries to free a buffer it did not allocate.
	static std::string HomePath;

	// Every call site below runs again on a filesystem restart or an fs_game change; their one-off
	// work must not.
	static bool Applied = false;
	static bool Announced = false;
	static bool FormatApplied = false;

	// Retail seals the block with XXTEA and an HMAC keyed off the CD key at 0x724B84, which the two
	// clients do not load from the same place: retail reads HKLM only (0x4FDE70), CoD4X prefers HKCU
	// (common.c:1646). Different keys mean the other client decodes noise, fails the digest and
	// quarantines the file, so CoD4X's unkeyed format is written instead. The saveStatData_t arrives
	// in eax, hence a stub rather than a hook.
	ASM_FUNCTION(LiveStorage_EncodeStatsData)
	{
		using namespace asmjit;

		a.push(x86::esi);
		a.mov(x86::esi, x86::eax);

		a.mov(x86::dword_ptr(x86::esi, MagicField), PlainMagic);
		a.call(x86::dword_ptr(SysTimeGetTime));
		a.mov(x86::dword_ptr(x86::esi, SaveTimeField), x86::eax);

		// Nothing reads the digest in this format, and zeroing beats shipping whatever the stack held.
		a.xor_(x86::eax, x86::eax);
		for (int32_t offset = 0; offset < DigestSize; offset += sizeof(uint32_t))
			a.mov(x86::dword_ptr(x86::esi, DigestField + offset), x86::eax);

		a.pop(x86::esi);
		a.ret();
	}

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

	// Retail splits the profile directory between both roots: save file I/O goes through fs_homepath,
	// but the "players" search path entry and the profile create/exists check are built from
	// fs_basepath. A stock install has the two naming the same directory, so nothing ever noticed.
	static void UseHomePathForProfiles()
	{
		Memory::Set<uint32_t>(0x4FB021, HomePathDvar);
		Memory::Set<uint32_t>(0x4FB0BC, HomePathDvar);
		Memory::Set<uint32_t>(0x55E6F9, HomePathDvar);
	}

	// FS_ReplaceSeparators runs over both sides first, so either separator matches.
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

	// An 'ice0' file carries no signature by design, so only the fs_game it was written under is
	// checked, as CoD4X does. Every other magic keeps retail's own verification.
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

	// Harmless under CoD4X, which replaces LiveStorage_UploadStats and never reaches retail's encoder.
	void Profile::UseCoD4XStatsFormat()
	{
		if (std::exchange(FormatApplied, true))
			return;

		Memory::JMP(EncodeStatsData, ASM_LOAD(LiveStorage_EncodeStatsData));
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
		// Cvar_RegisterString has already applied any +set, so a current value differing from the
		// default is a deliberate choice and outranks this.
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
