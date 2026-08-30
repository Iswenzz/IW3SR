#include "Profile.hpp"
#include "Dvar.hpp"
#include "Patch.hpp"

using namespace asmjit;

namespace IW3SR
{
	constexpr std::string_view SaveFolder = "CallofDuty4MW";

	// CoD4X's stats format: plaintext and unsigned (cl_main.c:5115), unlike retail's sealed 'iwm0'.
	constexpr uint32_t PlainMagic = 0x30656369; // 'ice0'

	constexpr uintptr_t EncodeStatsData = 0x5794E0;
	constexpr uintptr_t SysTimeGetTime = 0x69136C;

	constexpr int32_t MagicField = offsetof(saveStatData_t, magic);
	constexpr int32_t SaveTimeField = offsetof(saveStatData_t, saveTime);
	constexpr int32_t DigestField = offsetof(saveStatData_t, digest);
	constexpr int32_t DigestSize = sizeof(saveStatData_t::digest);

	// Every patched site loads a dvar_s* and reads its current value from this offset.
	constexpr int32_t DvarValueField = offsetof(dvar_s, current);
	static_assert(DvarValueField == 0x0C, "The engine reads dvar->current at +0x0C.");

	// stats+0x12F holds the stat_version the file was written under, and a mismatch wipes the block
	// and opens MENU_RESETCUSTOMCLASSES. Nothing is migrated on the way, so a bump only ever destroys.
	constexpr uintptr_t StatsVersionCheck = 0x579AD9;

	// Sites where the engine builds a per-user path. CoD4X reaches this same set by forking the
	// FS_SV family into *SavePath twins and switching individual callers (files.c:633 onward); these
	// are those callers, reached instead by pointing each dvar load at our own slot.
	constexpr uintptr_t SavePathOperands[] = {
		0x4FB021,
		0x4FB0BC, // the profile create and exists checks
		0x55B2D2,
		0x55B2FE, // FS_SV_Rename, reached only from LiveStorage_HandleCorruptStats
		0x55C0E9, // FS_SV_Remove, the same
		0x502D2E, // FS_SV_FOpenFileRead probes here before anywhere else, and
		0x502DB3, // compares against fs_basepath, so the install is still searched second
		0x55E882,
		0x55E8B8, // FS_Startup's second game directory root, which is how the save path
		0x55E946,
		0x55E9FA, // contributes its main and mods iwds to the search path
	};

	// FS_FOpenFileWrite is shared: the config and stats writes below move, while the demo, sound and
	// shock writes that also reach it stay in the install. Its callers decide, not the function.
	constexpr uintptr_t FileWriteCallers[] = {
		0x4FFAC3,
		0x4FFB5A, // Com_WriteConfiguration
		0x55C4EE,
		0x55C55E, // FS_WriteFileToDir, whose only callers are active.txt and mpdata
	};

	// FS_SV_FOpenFileWrite is shared the same way, between servercache.dat and the download temp.
	constexpr uintptr_t ServerCacheWrite = 0x476549;

	// FS_Startup adds "players" from fs_basepath alone, and active.txt is read back through the
	// search path rather than through FS_SV, so the save path has to be on it or no profile is ever
	// found. CoD4X reaches the same place from the other side, replacing the read and the profile
	// listing with FS_SV_FOpenFileRead and FS_SV_ListDirectories, which merge all three roots.
	constexpr uintptr_t AddPlayersCall = 0x55E706;
	constexpr uintptr_t AddGameDirectory = 0x55E020;
	constexpr uintptr_t PlayersDir = 0x6E0D7C; // "players"

	// Both write functions load fs_homepath in their prologue, so a twin only has to repeat that
	// prologue with the save path and re-enter the body. Copying the bodies would mean relocating
	// their relative branches for no gain.
	constexpr uintptr_t FileWriteBody = 0x55B4D9;
	constexpr uintptr_t SvFileWriteBody = 0x502BFD;

	// Registered as a real dvar the way CoD4X registers fs_savepath, so fs_homepath and fs_basepath
	// keep meaning the install directory and only the sites above look anywhere else.
	static dvar_s* SavePathDvar = nullptr;
	static std::string SavePath;

	// Every call site below runs again on a filesystem restart or an fs_game change; their one-off
	// work must not.
	static bool Applied = false;
	static bool Announced = false;
	static bool FormatApplied = false;

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

	static bool Resolve(std::filesystem::path& path)
	{
		const char* local = std::getenv("LOCALAPPDATA");
		if (!local || !local[0])
			return false;

		path = std::filesystem::path(local) / SaveFolder;

		std::error_code error;
		std::filesystem::create_directories(path, error);
		return !error;
	}

	// Retail seals the block with XXTEA and an HMAC keyed off the CD key at 0x724B84, which the two
	// clients do not load from the same place: retail reads HKLM only (0x4FDE70), CoD4X prefers HKCU
	// (common.c:1646). Different keys mean the other client decodes noise, fails the digest and
	// quarantines the file, so CoD4X's unkeyed format is written instead. The saveStatData_t arrives
	// in eax, hence a stub rather than a hook.
	ASM_FUNCTION(LiveStorage_EncodeStatsData)
	{
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

	ASM_FUNCTION(FS_FOpenFileWriteSavePath)
	{
		a.mov(x86::ecx, x86::dword_ptr(reinterpret_cast<uint64_t>(&SavePathDvar)));
		a.mov(x86::edx, x86::dword_ptr(x86::ecx, DvarValueField));
		a.jmp(FileWriteBody);
	}

	// One instruction more than its sibling: the filename is a stack argument here.
	ASM_FUNCTION(FS_SV_FOpenFileWriteSavePath)
	{
		a.mov(x86::eax, x86::dword_ptr(x86::esp, 0x04));
		a.mov(x86::ecx, x86::dword_ptr(reinterpret_cast<uint64_t>(&SavePathDvar)));
		a.mov(x86::edx, x86::dword_ptr(x86::ecx, DvarValueField));
		a.jmp(SvFileWriteBody);
	}

	// Called in place of the engine's single add, with the root it computed still on the stack. The
	// save path goes on second because the search path is walked newest first, which is the order
	// CoD4X reads these roots in.
	ASM_FUNCTION(FS_AddPlayersDirectories)
	{
		a.push(x86::dword_ptr(x86::esp, 0x04));
		a.mov(x86::edi, PlayersDir);
		a.call(AddGameDirectory);
		a.add(x86::esp, 0x04);

		a.mov(x86::ecx, x86::dword_ptr(reinterpret_cast<uint64_t>(&SavePathDvar)));
		a.mov(x86::eax, x86::dword_ptr(x86::ecx, DvarValueField));
		a.push(x86::eax);
		a.mov(x86::edi, PlayersDir);
		a.call(AddGameDirectory);
		a.add(x86::esp, 0x04);

		a.ret();
	}

	static void UseSavePath()
	{
		const auto slot = reinterpret_cast<uint32_t>(&SavePathDvar);
		for (uintptr_t operand : SavePathOperands)
			Memory::Set<uint32_t>(operand, slot);

		const uintptr_t fileWrite = ASM_LOAD(FS_FOpenFileWriteSavePath);
		for (uintptr_t caller : FileWriteCallers)
			Memory::CALL(caller, fileWrite);

		Memory::CALL(ServerCacheWrite, ASM_LOAD(FS_SV_FOpenFileWriteSavePath));
		Memory::CALL(AddPlayersCall, ASM_LOAD(FS_AddPlayersDirectories));
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

		std::filesystem::path path;
		if (!Resolve(path))
		{
			Log::WriteLine(Channel::Error, "Could not resolve %LOCALAPPDATA%; per-user files stay in the install.");
			return;
		}
		// Assigned once: a filesystem restart runs this again, and the engine may still be holding
		// the pointer handed to it the first time.
		if (SavePath.empty())
			SavePath = path.string();

		// A +set on the command line has already been applied by the time this returns, so an
		// explicit choice outranks the default without needing a check of its own.
		// DVAR_WRITEPROTECTED is BIT(4), the CVAR_INIT CoD4X registers fs_savepath with.
		SavePathDvar = Dvar::RegisterString("fs_savepath", DVAR_WRITEPROTECTED,
			"Where per-user files live: profiles, config and stats", SavePath.c_str());

		if (!SavePathDvar)
		{
			Log::WriteLine(Channel::Error, "fs_savepath could not be registered; per-user files stay in the install.");
			return;
		}

		if (std::exchange(Applied, true))
			return;

		UseSavePath();
		Memory::Set<uint8_t>(StatsVersionCheck, 0xEB);
		BackupStats(path / "players" / "profiles");

		Log::WriteLine(Channel::Game, "fs_savepath is {}.", SavePathDvar->current.string);
	}
}
