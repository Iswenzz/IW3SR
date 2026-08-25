#include "Assets.hpp"

#include "Game/System/Dvar.hpp"

namespace IW3SR
{
	// Zones the loader resolves through paths this lookup does not model, either because they
	// ship with the game or because they come from the patch directory. Left to the engine.
	constexpr std::array<std::string_view, 7> ReservedZones = { "code_post_gfx_mp", "localized_code_post_gfx_mp",
		"ui_mp", "common_mp", "localized_common_mp", "mp_patch", "cod4x_patchv2" };

	void Assets::Initialize()
	{
		IgnoreMissingZones = Dvar::RegisterBool("sr_ignore_missing_zones", DVAR_SAVED,
			"Skip fastfiles that are not installed instead of dropping to the error screen", true);
	}

	void Assets::LoadXAssets(XZoneInfo* zoneInfo, unsigned int zoneCount, int sync)
	{
		// Boot zones are loaded before the dvar exists, default to skipping there too.
		const bool ignore = !IgnoreMissingZones || IgnoreMissingZones->current.enabled;

		if (!zoneInfo || !ignore)
		{
			DB_LoadXAssets_h(zoneInfo, zoneCount, sync);
			return;
		}

		// The caller usually hands us a static array, so patch a copy of it.
		std::vector<XZoneInfo> zones(zoneInfo, zoneInfo + zoneCount);

		for (auto& zone : zones)
		{
			if (!zone.name || IsReserved(zone.name) || ZoneExists(zone.name))
				continue;

			const std::string name = zone.name;
			zone.name = nullptr;

			if (std::ranges::find(Skipped, name) == Skipped.end())
				Skipped.push_back(name);

			Com_PrintMessage(CON_CHANNEL_FILES,
				std::format("^3Fastfile '{}' is not installed, skipping it. What it provides falls back to the "
							"default assets.\n",
					name)
					.c_str(),
				0);
		}
		DB_LoadXAssets_h(zones.data(), zoneCount, sync);
	}

	bool Assets::ZoneExists(const std::string& name)
	{
		if (name.empty())
			return false;

		// The mod zone is not looked up through the search paths, it is opened straight
		// out of the active fs_game directory.
		if (name == "mod")
			return std::filesystem::exists(ModZonePath());

		return DB_FileExists(name.c_str(), DB_PATH_ZONE) || DB_FileExists(name.c_str(), DB_PATH_MAIN)
			|| DB_FileExists(name.c_str(), DB_PATH_USERMAPS);
	}

	bool Assets::IsReserved(const std::string& name)
	{
		return std::ranges::find(ReservedZones, name) != ReservedZones.end();
	}

	void Assets::Reset()
	{
		Skipped.clear();
	}

	std::string Assets::ModZonePath()
	{
		const auto basepath = Dvar::Find("fs_basepath");
		const auto game = Dvar::Find("fs_game");

		if (!basepath || !game || !basepath->current.string || !game->current.string || !game->current.string[0])
			return {};

		return std::format("{}/{}/mod.ff", basepath->current.string, game->current.string);
	}
}
