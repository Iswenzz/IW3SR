#include "Zones.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

namespace IW3SR
{
	// DB_LoadXZone copies at most eight named zones out of a batch and silently drops the rest.
	constexpr size_t MaxBatchZones = 8;
	constexpr size_t MaxLayerZones = 4;

	// XZONE_MOD would be the obvious flag, but the engine frees mod zones every time fs_game
	// changes, which is exactly when a patch layer has to stay put.
	constexpr int LayerFlags = XZONE_COMMON;

	// Keeps a stray fastfile dropped in the folder from shadowing a retail zone by name.
	constexpr std::string_view LayerPrefix = "iw3sr_";

	// Where CoD4x downloads its own patch zone to; under CoD4x that loader is the one serving it.
	constexpr std::string_view CoD4XZonePath = "CallofDuty4MW/zone";

	// FS_CompareFFs measures every fastfile the server references with DB_FileSize, and the sizes are
	// what sv_referencedFFCheckSums actually carries. DB_FileSize only ever builds
	// <install>/zone/<language>/<name>.ff, but CoD4x keeps its own zones a level up - so on an
	// extended server cod4x_patchv2 measures zero, and CL_InitDownloads drops the connection with
	// "cod4x_patchv2.ff is different from the server". CoD4x replaces the same call
	// (CoD4x_Client_pub/src/sys_patch.c:1112).
	//   005039d1: e8 6a 7f f8 ff   call 0x48b940
	constexpr uintptr_t FileSizeSite = 0x5039D1;
	constexpr uintptr_t FileSizeTarget = 0x48B940;

	// Runs on every CreateFileA in the process, and the names are ASCII, so no locale-aware tolower.
	static constexpr char Lower(char value)
	{
		return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
	}

	static bool EqualsNoCase(std::string_view left, std::string_view right)
	{
		return std::ranges::equal(left, right, [](char a, char b) { return Lower(a) == Lower(b); });
	}

	// Discovery is deferred to the first batch: it is loaded from inside R_Init, before anything in
	// IW3SR that runs off the renderer has been initialized.
	void GZones::Inject(std::vector<XZoneInfo>& zones)
	{
		if (!Discovered)
		{
			Discovered = true;
			Discover();
		}
		if (Injected || Layer.empty())
			return;

		// Ride the batch that outlives a map, the one bringing in common_mp: a map or mod batch
		// would take the layer back down on the next load.
		const bool common = std::ranges::any_of(zones,
			[](const XZoneInfo& zone) { return zone.name && (zone.allocFlags & XZONE_COMMON); });

		if (!common)
			return;

		size_t used = static_cast<size_t>(
			std::ranges::count_if(zones, [](const XZoneInfo& zone) { return zone.name != nullptr; }));

		for (const PatchZone& zone : Layer)
		{
			if (used >= MaxBatchZones)
			{
				Com_PrintMessage(CON_CHANNEL_FILES,
					std::format("^3No room left in the boot batch for zone '{}'.\n", zone.Name).c_str(), 0);
				break;
			}
			zones.push_back({ zone.Name.c_str(), LayerFlags, 0 });
			used++;

			Com_PrintMessage(CON_CHANNEL_FILES, std::format("Mounting IW3SR zone '{}'\n", zone.Path.string()).c_str(),
				0);
		}
		Injected = true;
	}

	void GZones::Shutdown()
	{
		Redirect.Remove();
	}

	std::filesystem::path GZones::Root()
	{
		return Environment::Initialized ? Environment::Path(Directory::App) / "Zone" : std::filesystem::path();
	}

	// FS_AddIwdFilesForGameDirectory walks <path>/<folder> and adds every .iwd to the search path,
	// the way the engine does for a mod folder. CoD4X mounts its own IWD the same way, so its search
	// path already covers this and a second mount would only duplicate entries.
	void GZones::MountIwd()
	{
		if (Patch::UseCoD4X || !FS_AddIwdFilesForGameDirectory || !Environment::Initialized)
			return;

		const std::filesystem::path app = Environment::Path(Directory::App);
		if (app.empty() || !app.has_parent_path())
			return;

		std::error_code ec;
		if (!std::filesystem::exists(app, ec) || ec)
			return;

		const std::string path = app.parent_path().string();
		const std::string folder = app.filename().string();

		FS_AddIwdFilesForGameDirectory(path.c_str(), folder.c_str());
	}

	// Runs behind the engine's own lookup and only ever turns a zero into a real size, so a zone the
	// engine already found keeps whatever it found and a stock session is untouched - a retail server
	// never references a name that is not already under zone/<language>.
	int GZones::FileSize(const char* name, int size)
	{
		if (size || !name || !*name)
			return size;

		const std::string file = std::string(name) + ".ff";

		const auto measure = [](const std::filesystem::path& candidate) -> int
		{
			std::error_code ec;
			const uintmax_t bytes = std::filesystem::file_size(candidate, ec);

			return !ec && bytes <= INT_MAX ? static_cast<int>(bytes) : 0;
		};

		for (const char* name : { "fs_savepath", "fs_homepath", "fs_basepath" })
		{
			const dvar_s* path = Dvar::Find(name);
			if (!path || !path->current.string)
				continue;

			if (const int bytes = measure(std::filesystem::path(path->current.string) / "zone" / file))
				return bytes;
		}

		// Where the updater writes, for the case where neither client registered fs_savepath.
		char local[MAX_PATH] = {};
		if (GetEnvironmentVariableA("LOCALAPPDATA", local, MAX_PATH))
			return measure(std::filesystem::path(local) / CoD4XZonePath / file);

		return 0;
	}

	// Refused unless the call still goes where stock 1.7 sends it, so a binary another mod already
	// sits on is left alone rather than redirected twice.
	void GZones::PatchFileSize()
	{
		if (Patch::UseCoD4X)
			return;
		if (Memory::Get<uint8_t>(FileSizeSite) != 0xE8
			|| FileSizeSite + 5 + Memory::Get<int32_t>(FileSizeSite + 1) != FileSizeTarget)
			return;

		Memory::CALL(FileSizeSite, ASM_LOAD(DB_FileSize_h));
	}

	void GZones::Discover()
	{
		// Independent of the layer below: this is what lets an extended server's own zones be
		// measured at all, and it has to happen even with sr_zones off.
		PatchFileSize();

		// Registered here rather than at renderer init, where the first batch is already loading.
		// The anchor dvar covers the table not being up yet: a hook must not take the boot down.
		if (Dvar::Find("fs_basepath"))
		{
			Enabled = Dvar::RegisterBool("sr_zones", DVAR_SAVED,
				"Mount the fastfiles in IW3SR/Zone on top of the retail zones", true);
		}
		if (Enabled && !Enabled->current.enabled)
			return;

		MountIwd();

		Collect();
		if (Layer.empty())
			return;

		// CoD4x already replaces the whole zone file open path, private-zone lookup included, so the
		// layer is left to it and only the zones its loader can reach are kept.
		if (Patch::UseCoD4X)
		{
			std::erase_if(Layer, [](const PatchZone& zone) { return !Resolvable(zone); });
			return;
		}

		const HMODULE kernel = GetModuleHandleA("kernel32.dll");
		const FARPROC target = kernel ? GetProcAddress(kernel, "CreateFileA") : nullptr;

		if (target)
		{
			Redirect.Callback = reinterpret_cast<uint64_t>(&GZones::OpenFile);
			Redirect.Update(reinterpret_cast<uintptr_t>(target));
		}

		// Without the redirect the engine only looks in its own zone folder, and a zone it cannot
		// reach aborts the load with a missing zone error.
		if (!Redirect.IsEnabled)
			std::erase_if(Layer, [](const PatchZone& zone) { return !Resolvable(zone); });
	}

	void GZones::Collect()
	{
		const std::filesystem::path root = Root();

		std::error_code ec;
		if (root.empty() || !std::filesystem::is_directory(root, ec))
			return;

		std::vector<std::filesystem::path> files;

		for (const auto& entry : std::filesystem::directory_iterator(root, ec))
		{
			const std::filesystem::path& path = entry.path();

			if (!entry.is_regular_file(ec) || !EqualsNoCase(path.extension().string(), ".ff"))
				continue;

			const std::string stem = path.stem().string();
			if (stem.size() <= LayerPrefix.size() || !EqualsNoCase(stem.substr(0, LayerPrefix.size()), LayerPrefix))
				continue;

			files.push_back(path);
		}
		std::ranges::sort(files, [](const std::filesystem::path& left, const std::filesystem::path& right)
			{ return left.native() < right.native(); });

		for (const std::filesystem::path& file : files)
		{
			if (Layer.size() >= MaxLayerZones)
			{
				Com_PrintMessage(CON_CHANNEL_FILES,
					std::format("^3IW3SR/Zone holds more than {} fastfiles, the rest are ignored.\n", MaxLayerZones)
						.c_str(),
					0);
				break;
			}
			Layer.push_back({ file.stem().string(), file.filename().string(), file });
		}
	}

	bool GZones::Resolvable(const PatchZone& zone)
	{
		if (DB_FileExists(zone.Name.c_str(), DB_PATH_ZONE) || DB_FileExists(zone.Name.c_str(), DB_PATH_MAIN))
			return true;

		char local[MAX_PATH] = {};
		if (!GetEnvironmentVariableA("LOCALAPPDATA", local, MAX_PATH))
			return false;

		std::error_code ec;
		if (std::filesystem::exists(std::filesystem::path(local) / CoD4XZonePath / zone.File, ec))
			return true;

		Com_PrintMessage(CON_CHANNEL_FILES,
			std::format("^3Zone '{}' cannot be reached from IW3SR/Zone here, copy it to %LOCALAPPDATA%/{}.\n",
				zone.File, CoD4XZonePath)
				.c_str(),
			0);
		return false;
	}

	const PatchZone* GZones::Match(std::string_view path)
	{
		if (path.size() < 4 || !EqualsNoCase(path.substr(path.size() - 3), ".ff"))
			return nullptr;

		const size_t separator = path.find_last_of("/\\");
		const std::string_view file = separator == std::string_view::npos ? path : path.substr(separator + 1);

		for (const PatchZone& zone : Layer)
		{
			if (EqualsNoCase(file, zone.File))
				return &zone;
		}
		return nullptr;
	}

	// Only the path is swapped: the access, sharing and overlapped flags the caller picked are what
	// the loader needs back.
	HANDLE GZones::OpenFile(LPCSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation,
		DWORD flags, HANDLE templ)
	{
		const PatchZone* zone = path ? Match(path) : nullptr;

		if (!zone)
			return Redirect(path, access, share, security, creation, flags, templ);

		return CreateFileW(zone->Path.c_str(), access, share, security, creation, flags, templ);
	}
}
