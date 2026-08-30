#include "AssetDump.hpp"

#include "Game/System/Dvar.hpp"

#include "Engine/Core/IO/Zip.hpp"

namespace IW3SR
{
	constexpr uint32_t HashTableSize = 0x8000;
	constexpr uint32_t PoolSize = 0x8000;

	constexpr int MaxZones = ASSET_TYPE_COUNT;

	constexpr std::array<std::string_view, ASSET_TYPE_COUNT> TypeNames = { "xmodelpieces", "physpreset", "xanim",
		"xmodel", "material", "techset", "image", "sound", "sndcurve", "loaded_sound", "col_map_sp", "col_map_mp",
		"com_map", "game_map_sp", "game_map_mp", "map_ents", "gfx_map", "lightdef", "ui_map", "font", "menufile",
		"menu", "localize", "weapon", "snddriverglobals", "fx", "impactfx", "aitype", "mptype", "character",
		"xmodelalias", "rawfile", "stringtable" };

	static bool Contains(std::string_view haystack, std::string_view needle)
	{
		if (needle.empty())
			return true;
		if (needle.size() > haystack.size())
			return false;

		const auto lower = [](char value)
		{ return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value; };

		for (size_t i = 0; i + needle.size() <= haystack.size(); i++)
		{
			if (std::ranges::equal(haystack.substr(i, needle.size()), needle,
					[&](char a, char b) { return lower(a) == lower(b); }))
			{
				return true;
			}
		}
		return false;
	}

	void GAssetDump::Initialize()
	{
		Developer = Dvar::RegisterBool("sr_dev_assets", DVAR_NONE,
			"Enable the xasset inspection commands, sr_assets and sr_assets_dump", false);
		Archive = Dvar::RegisterBool("sr_dev_assets_zip", DVAR_SAVED,
			"Pack each dump into a single zip and drop the folder it was written from", false);
	}

	bool GAssetDump::Command(const std::string& command)
	{
		std::istringstream stream(command);
		std::string name;
		stream >> name;

		if (name != "sr_assets" && name != "sr_assets_dump")
			return false;

		std::string type, filter;
		stream >> type >> filter;

		if (!Allowed())
			return true;

		std::optional<XAssetType> selected;
		if (!type.empty() && type != "*")
		{
			selected = Parse(type);
			if (!selected)
			{
				Com_PrintMessage(CON_CHANNEL_ERROR, std::format("^1Unknown asset type '{}'.\n", type).c_str(), 0);
				return true;
			}
		}

		if (name == "sr_assets")
			List(selected, filter);
		else
			Dump(selected, filter);
		return true;
	}

	bool GAssetDump::Allowed()
	{
		if (Developer && Developer->current.enabled)
			return true;

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "^3Set sr_dev_assets 1 to use the xasset commands.\n", 0);
		return false;
	}

	std::optional<XAssetType> GAssetDump::Parse(const std::string& name)
	{
		for (size_t i = 0; i < TypeNames.size(); i++)
		{
			if (TypeNames[i] == name)
				return static_cast<XAssetType>(i);
		}
		return std::nullopt;
	}

	// Walks the database the way DB_EnumXAssets does, but takes no db_hashCritSect, so it is only
	// safe from the console with no zone load in flight.
	std::vector<AssetRecord> GAssetDump::Collect(std::optional<XAssetType> type, const std::string& filter)
	{
		std::vector<AssetRecord> records;

		if (!db_hashTable || !g_assetEntryPool)
			return records;

		// A chain walked off a torn hash table would never come back, so the walk is bounded.
		uint32_t budget = PoolSize;

		for (uint32_t hash = 0; hash < HashTableSize && budget; hash++)
		{
			for (uint32_t index = db_hashTable[hash]; index && budget; index = g_assetEntryPool[index].nextHash)
			{
				if (index >= PoolSize)
					break;
				budget--;

				const XAssetEntry& entry = g_assetEntryPool[index];
				Take(records, entry, type, filter);

				for (uint32_t alternate = entry.nextOverride; alternate && budget;
					 alternate = g_assetEntryPool[alternate].nextOverride)
				{
					if (alternate >= PoolSize)
						break;
					budget--;

					Take(records, g_assetEntryPool[alternate], type, filter);
				}
			}
		}
		std::ranges::sort(records, [](const AssetRecord& left, const AssetRecord& right)
			{ return left.Type != right.Type ? left.Type < right.Type : left.Name < right.Name; });

		return records;
	}

	void GAssetDump::Take(std::vector<AssetRecord>& records, const XAssetEntry& entry, std::optional<XAssetType> type,
		const std::string& filter)
	{
		if (!entry.inuse || static_cast<uint32_t>(entry.asset.type) >= ASSET_TYPE_COUNT)
			return;
		if (type && entry.asset.type != *type)
			return;

		const char* name = Name(entry.asset);
		if (!name || !Contains(name, filter))
			return;

		records.push_back({ entry.asset.type, name, entry.zoneIndex, entry.asset.header });
	}

	void GAssetDump::List(std::optional<XAssetType> type, const std::string& filter)
	{
		const std::vector<AssetRecord> records = Collect(type, filter);

		if (!type)
		{
			std::array<int, ASSET_TYPE_COUNT> counts = {};
			for (const AssetRecord& record : records)
				counts[record.Type]++;

			for (size_t i = 0; i < counts.size(); i++)
			{
				if (counts[i])
					Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
						std::format("{:<18} {:>6} / {}\n", TypeName(static_cast<XAssetType>(i)), counts[i],
							g_poolSize ? g_poolSize[i] : 0)
							.c_str(),
						0);
			}
		}
		else
		{
			for (const AssetRecord& record : records)
				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
					std::format("{} ({})\n", record.Name, ZoneName(record.Zone)).c_str(), 0);
		}

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, std::format("{} assets.\n", records.size()).c_str(), 0);
	}

	void GAssetDump::Dump(std::optional<XAssetType> type, const std::string& filter)
	{
		if (!Environment::Initialized)
			return;

		const std::vector<AssetRecord> records = Collect(type, filter);
		if (records.empty())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Nothing matched, nothing written.\n", 0);
			return;
		}

		SYSTEMTIME now = {};
		GetLocalTime(&now);

		const std::string stamp = std::format("{:04}{:02}{:02}-{:02}{:02}{:02}", now.wYear, now.wMonth, now.wDay,
			now.wHour, now.wMinute, now.wSecond);
		const std::filesystem::path root = Environment::Path(Directory::App) / "Dumps" / stamp;

		std::error_code ec;
		std::filesystem::create_directories(root, ec);

		if (!Index(root, records))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR,
				std::format("^1Could not write the dump index to {}.\n", root.string()).c_str(), 0);
			return;
		}

		int written = 0;
		for (const AssetRecord& record : records)
			written += Write(root, record) ? 1 : 0;

		std::filesystem::path result = root;

		if (Archive && Archive->current.enabled)
		{
			const std::filesystem::path archive = root.string() + ".zip";
			if (Zip::Compress(root, archive))
			{
				std::filesystem::remove_all(root, ec);
				result = archive;
			}
		}

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Indexed {} assets, wrote {} of them to {}\n", records.size(), written, result.string())
				.c_str(),
			0);
	}

	bool GAssetDump::Index(const std::filesystem::path& root, const std::vector<AssetRecord>& records)
	{
		std::ofstream file(root / "index.csv", std::ios::trunc);
		if (!file.is_open())
			return false;

		file << "type,name,zone\n";
		for (const AssetRecord& record : records)
			file << TypeName(record.Type) << ",\"" << record.Name << "\"," << ZoneName(record.Zone) << '\n';

		return true;
	}

	// Only the types whose payload is plain bytes behind a length. The rest need the full CoD4x
	// serializer (xasset_loader.c:360 DumpXAsset), a per-type walk of the whole asset graph.
	bool GAssetDump::Write(const std::filesystem::path& root, const AssetRecord& record)
	{
		if (!record.Header.data)
			return false;

		std::string listing;
		const char* data = nullptr;
		int length = 0;
		std::string extension;

		switch (record.Type)
		{
		case ASSET_TYPE_RAWFILE:
			data = record.Header.rawfile->buffer;
			length = record.Header.rawfile->len;
			break;

		case ASSET_TYPE_MAP_ENTS:
			data = record.Header.mapEnts->entityString;
			length = record.Header.mapEnts->numEntityChars;
			extension = ".ents";
			break;

		case ASSET_TYPE_MENULIST:
		{
			const MenuList* list = record.Header.menuList;
			if (!list->menus)
				return false;

			for (int i = 0; i < list->menuCount; i++)
			{
				const menuDef_t* menu = list->menus[i];
				listing += menu && menu->window.name ? menu->window.name : "?";
				listing += '\n';
			}
			data = listing.data();
			length = static_cast<int>(listing.size());
			extension = ".txt";
			break;
		}

		default:
			return false;
		}

		if (!data || length <= 0)
			return false;

		const std::filesystem::path path = root / TypeName(record.Type) / (Safe(record.Name) + extension);

		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			return false;

		file.write(data, length);
		return true;
	}

	// Most asset headers open with their name, but not all do, so the types with no struct are left
	// out rather than read at a guessed offset.
	const char* GAssetDump::Name(const XAsset& asset)
	{
		if (!asset.header.data)
			return nullptr;

		switch (asset.type)
		{
		case ASSET_TYPE_PHYSPRESET:
			return asset.header.physPreset->name;
		case ASSET_TYPE_XANIMPARTS:
			return asset.header.parts->name;
		case ASSET_TYPE_XMODEL:
			return asset.header.model->name;
		case ASSET_TYPE_MATERIAL:
			return asset.header.material->info.name;
		case ASSET_TYPE_TECHNIQUE_SET:
			return asset.header.techniqueSet->name;
		case ASSET_TYPE_IMAGE:
			return asset.header.image->name;
		case ASSET_TYPE_CLIPMAP:
		case ASSET_TYPE_CLIPMAP_PVS:
			return asset.header.clipMap->name;
		case ASSET_TYPE_COMWORLD:
			return asset.header.comWorld->name;
		case ASSET_TYPE_GAMEWORLD_SP:
			return asset.header.gameWorldSp->name;
		case ASSET_TYPE_GAMEWORLD_MP:
			return asset.header.gameWorldMp->name;
		case ASSET_TYPE_MAP_ENTS:
			return asset.header.mapEnts->name;
		case ASSET_TYPE_GFXWORLD:
			return asset.header.gfxWorld->name;
		case ASSET_TYPE_LIGHT_DEF:
			return asset.header.lightDef->name;
		case ASSET_TYPE_FONT:
			return asset.header.font->fontName;
		case ASSET_TYPE_MENULIST:
			return asset.header.menuList->name;
		case ASSET_TYPE_MENU:
			return asset.header.menu->window.name;
		case ASSET_TYPE_WEAPON:
			return asset.header.weapon->szInternalName;
		case ASSET_TYPE_FX:
			return asset.header.fx->name;
		case ASSET_TYPE_RAWFILE:
			return asset.header.rawfile->name;
		default:
			return nullptr;
		}
	}

	std::string_view GAssetDump::TypeName(XAssetType type)
	{
		return static_cast<uint32_t>(type) < ASSET_TYPE_COUNT ? TypeNames[type] : "unknown";
	}

	std::string GAssetDump::ZoneName(int zone)
	{
		if (!g_zones || zone < 0 || zone >= MaxZones || !g_zones[zone].name[0])
			return "-";

		return g_zones[zone].name;
	}

	// Rawfile names carry folders worth keeping; nothing that could walk the dump out of its own directory is.
	std::string GAssetDump::Safe(const std::string& name)
	{
		std::string result;
		result.reserve(name.size());

		for (size_t i = 0; i < name.size(); i++)
		{
			const char value = name[i];

			if (value == '\\' || value == '/')
			{
				if (!result.empty() && result.back() != '/')
					result += '/';
				continue;
			}
			if (value == ':' || value == '*' || value == '?' || value == '"' || value == '<' || value == '>'
				|| value == '|' || static_cast<unsigned char>(value) < 0x20)
			{
				result += '_';
				continue;
			}
			if (value == '.' && i + 1 < name.size() && name[i + 1] == '.')
			{
				result += '_';
				continue;
			}
			result += value;
		}
		return result.empty() ? "unnamed" : result;
	}
}
