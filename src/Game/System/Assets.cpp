#include "Assets.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Zones.hpp"

namespace IW3SR
{
	constexpr std::array<std::string_view, 7> ReservedZones = { "code_post_gfx_mp", "localized_code_post_gfx_mp",
		"ui_mp", "common_mp", "localized_common_mp", "mp_patch", "cod4x_patchv2" };

	constexpr const char* DefaultWeapon = "defaultweapon_mp";

	constexpr int CS_ITEMS = 2314;
	constexpr int ItemCount = 128;

	static_assert(offsetof(clientActive_t, gameState) + CS_ITEMS * sizeof(int) == 0xC64D14 - 0xC5F930);

	void Assets::Initialize()
	{
		IgnoreMissingZones = Dvar::RegisterBool("sr_ignore_missing_zones", DVAR_SAVED,
			"Skip fastfiles that are not installed instead of dropping to the error screen", true);
	}

	void Assets::LoadXAssets(XZoneInfo* zoneInfo, unsigned int zoneCount, int sync)
	{
		const bool ignore = !IgnoreMissingZones || IgnoreMissingZones->current.enabled;

		if (!zoneInfo)
		{
			DB_LoadXAssets_h(zoneInfo, zoneCount, sync);
			return;
		}
		std::vector<XZoneInfo> zones(zoneInfo, zoneInfo + zoneCount);
		GZones::Inject(zones);

		if (ignore)
		{
			for (auto& zone : zones)
			{
				if (!zone.name || IsReserved(zone.name) || ZoneExists(zone.name))
					continue;

				const std::string name = zone.name;
				zone.name = nullptr;

				Com_PrintMessage(CON_CHANNEL_FILES,
					std::format("^3Fastfile '{}' is not installed, skipping it. What it provides falls back to the "
								"default assets.\n",
						name)
						.c_str(),
					0);
			}
		}
		DB_LoadXAssets_h(zones.data(), static_cast<unsigned int>(zones.size()), sync);
	}

	// The RCE CoD4X update servers fire at a stock 1.7 client to force the client update on it.
	// Retail copies CS_ITEMS into a 132 byte stack buffer with a loop that stops only on the
	// terminator, and that buffer ends exactly where the return address begins - a server sending a
	// long enough items configstring picks the address CG_RegisterItems returns to. Reimplemented
	// rather than bounded in place: the mask is read at [0, ItemCount / 4) and nothing past it is
	// ever wanted, so the copy is capped there and the rest never reaches the stack.
	void Assets::RegisterItems()
	{
		const gameState_t& gameState = clients->gameState;
		const char* items = &gameState.stringData[gameState.stringOffsets[CS_ITEMS]];

		// Zero filled: retail left the tail of its buffer uninitialised, so a configstring shorter
		// than the mask registered items off whatever the stack happened to hold.
		char mask[ItemCount / 4] = {};
		for (size_t i = 0; i < sizeof(mask) && items[i]; ++i)
			mask[i] = items[i];

		// One hex nibble per four items, low index first. Signed like retail's movsbl, so a byte the
		// server made negative lands on the same branch it always did.
		for (int i = 1; i < ItemCount; ++i)
		{
			const int nibble = mask[i / 4] > '9' ? mask[i / 4] - 'W' : mask[i / 4] - '0';
			if (nibble & (1 << (i & 3)))
				CG_RegisterItemVisuals(i);
		}
	}

	// The engine insists every weapon a demo names lands on the index the recording used. One the
	// mod no longer ships shifts every later weapon down and the demo is dropped, so the owed slot
	// is filled with the default weapon.
	void Assets::RegisterWeapons(const char** weapons, int weaponCount)
	{
		for (int i = 0; i < weaponCount; i++)
		{
			const int expected = i + 1;
			if (BG_RegisterWeapon(weapons[i], nullptr) == expected)
				continue;

			WeaponDef* fallback = BG_LoadWeaponDef(DefaultWeapon);
			if (!fallback)
				continue;

			while (bg_weaponCount < expected)
				BG_AddWeapon(fallback, nullptr);

			Com_PrintMessage(CON_CHANNEL_FILES,
				std::format("^3Weapon '{}' is not the one this demo was recorded with, using the default weapon.\n",
					weapons[i])
					.c_str(),
				0);
		}
	}

	bool Assets::ZoneExists(const std::string& name)
	{
		if (name.empty())
			return false;

		if (name == "mod")
			return std::filesystem::exists(ModZonePath());

		return DB_FileExists(name.c_str(), DB_PATH_ZONE) || DB_FileExists(name.c_str(), DB_PATH_MAIN)
			|| DB_FileExists(name.c_str(), DB_PATH_USERMAPS);
	}

	bool Assets::IsReserved(const std::string& name)
	{
		return std::ranges::find(ReservedZones, name) != ReservedZones.end();
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
