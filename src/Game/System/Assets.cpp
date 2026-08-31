#include "Assets.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Zones.hpp"

namespace IW3SR
{
	constexpr std::array<std::string_view, 7> ReservedZones = { "code_post_gfx_mp", "localized_code_post_gfx_mp",
		"ui_mp", "common_mp", "localized_common_mp", "mp_patch", "cod4x_patchv2" };

	constexpr const char* FallbackWeapon = "beretta_mp";
	constexpr const char* LastResortWeapon = "defaultweapon_mp";

	constexpr int AssetTypeXModel = 3;
	constexpr int AssetTypeFx = 25;
	static const char* const* const DefaultAssetNames = reinterpret_cast<const char* const*>(0x726678);

	constexpr const char* StockViewhands = "viewmodel_base_viewhands";

	static bool EqualsNoCase(std::string_view left, std::string_view right)
	{
		const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; };

		return left.size() == right.size()
			&& std::equal(left.begin(), left.end(), right.begin(),
				[&](char a, char b) { return lower(a) == lower(b); });
	}

	constexpr int CS_ITEMS = 2314;
	constexpr int ItemCount = 128;

	static_assert(offsetof(clientActive_t, gameState) + CS_ITEMS * sizeof(int) == 0xC64D14 - 0xC5F930);

	void Assets::Initialize()
	{
		IgnoreMissingZones = Dvar::RegisterBool("sr_ignore_missing_zones", DVAR_SAVED,
			"Skip fastfiles that are not installed instead of dropping to the error screen", true);
		HideMissingFx = Dvar::RegisterBool("sr_hide_missing_fx", DVAR_SAVED,
			"Draw nothing where an effect the loaded content does not have would be", true);
		SubstituteViewmodel = Dvar::RegisterBool("sr_substitute_viewhands", DVAR_SAVED,
			"Draw the stock hands when the ones a demo asks for are not installed", true);
	}

	// Nothing about the answer says it is a substitution: the DB clones the default entry under the
	// name that was asked for (DB_CloneXAssetEntry, 0x489D40), so the header comes back carrying that
	// name and reads as a hit. DB_IsXAssetDefault is the only thing that knows.
	// Zeroing the element counts is what makes the placeholder draw nothing. The effects a weapon file
	// names are resolved by the zone loader, which comes through here as well, so those are covered
	// too; a zone reload brings the asset back and the next miss zeroes it again.
	void* Assets::FindXAssetHeader(int type, const char* name)
	{
		void* header = DB_FindXAssetHeader_h(type, name);

		if (type == AssetTypeXModel)
			return SubstituteViewhands(header, name);

		if (type != AssetTypeFx || !header || !name || !HideMissingFx || !HideMissingFx->current.enabled)
			return header;

		// Either route to the placeholder: an effect the content does not have, or the placeholder
		// asked for under its own name, which is not a substitution and so not what DB_IsXAssetDefault
		// answers. Both end up drawing it, so both empty it.
		const char* placeholder = DefaultAssetNames[AssetTypeFx];

		if (!DB_IsXAssetDefault(AssetTypeFx, name) && !(placeholder && EqualsNoCase(name, placeholder)))
			return header;

		auto* effect = static_cast<FxEffectDef*>(header);

		if (effect->elemDefCountLooping || effect->elemDefCountOneShot || effect->elemDefCountEmission)
		{
			effect->elemDefCountLooping = 0;
			effect->elemDefCountOneShot = 0;
			effect->elemDefCountEmission = 0;

			Log::WriteLine(Channel::Warning, "Effect '{}' is missing; emptied so the placeholder draws nothing.", name);
		}
		return header;
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

	// An image whose .iwi cannot be read is fatal at one of the five sites that load one (0x616F2B):
	// it reports "Couldn't load image" through Com_Error, and on the fastfile loader thread that is a
	// message box and a dead process. The other sites already recover by handing the image the default
	// texture for its semantic, so doing it here gives every site that behaviour. The engine has
	// already named the file it could not open by the time this runs.
	bool Assets::LoadImageFromFile(GfxImage* image, void* reader)
	{
		if (Image_LoadFromFile_h(image, reader))
			return true;

		// Only 2D images have a default to fall back on, and a null texture would fault the renderer
		// long before anything drew it, so anything else still has to reach the error.
		if (!image || !Image_SetDefaultTexture(image))
			return false;

		Log::WriteLine(Channel::Warning, "Image '{}' could not be read; drawing the default texture.",
			image->name ? image->name : "?");
		return true;
	}

	// A missing xmodel becomes "void", which is empty, and for a body that is the wanted answer: the
	// character simply is not drawn. For the hands it is not. The gun is a stock model and still draws,
	// but it hangs off tag_weapon on the hands, and "void" has no bones - so the attachment keeps the
	// uninitialised transform the engine leaves behind when it warns "Part 'tag_weapon' not found",
	// which is the stretched polygon across the screen and the gun sitting away from the view.
	// cg_drawGun 0 hides exactly that set. Real hands put the gun back on a tag that exists.
	void* Assets::SubstituteViewhands(void* header, const char* name)
	{
		if (!header || !name || !SubstituteViewmodel || !SubstituteViewmodel->current.enabled)
			return header;

		// Retail names them viewhands_*, mods just as often viewmodel_hands_*. Checked before asking
		// the DB anything, because that question costs a lookup and almost no model is one of these.
		if (!std::string_view(name).contains("hands") || EqualsNoCase(name, StockViewhands))
			return header;

		// The DB clones the default entry under the name that was asked for (DB_CloneXAssetEntry,
		// 0x489D40), so a substituted asset carries the requested name and comparing the two names
		// never identifies one. Only the entry itself knows, and only this answers it.
		if (!DB_IsXAssetDefault(AssetTypeXModel, name) || DB_IsXAssetDefault(AssetTypeXModel, StockViewhands))
			return header;

		void* stock = DB_FindXAssetHeader_h(AssetTypeXModel, StockViewhands);
		if (!stock)
			return header;

		Log::WriteLine(Channel::Warning, "Viewhands '{}' are missing; drawing '{}' so the weapon still has a tag.",
			name, StockViewhands);
		return stock;
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

			const char* substitute = FallbackWeapon;
			WeaponDef* fallback = BG_LoadWeaponDef(substitute);

			if (!fallback)
			{
				substitute = LastResortWeapon;
				fallback = BG_LoadWeaponDef(substitute);
			}
			if (!fallback)
				continue;

			while (bg_weaponCount < expected)
				BG_AddWeapon(fallback, nullptr);

			Com_PrintMessage(CON_CHANNEL_FILES,
				std::format("^3Weapon '{}' is not the one this demo was recorded with, using '{}'.\n", weapons[i],
					substitute)
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
