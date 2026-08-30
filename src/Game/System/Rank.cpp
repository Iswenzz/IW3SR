#include "Rank.hpp"
#include "Dvar.hpp"
#include "Patch.hpp"

#include <cstring>

namespace IW3SR
{
	constexpr size_t MaxPathLength = 128;
	constexpr size_t MaxNameLength = 32;

	static_assert(MaxNameLength + sizeof("mp/rankIconTable.csv") <= MaxPathLength);

	constexpr std::array<uintptr_t, 2> RankTableSites = { 0x4749F2, 0x474B2F };
	constexpr uintptr_t RankIconTableSite = 0x474B93;

	constexpr uintptr_t StockRankTable = 0x6D2AD0;
	constexpr uintptr_t StockRankIconTable = 0x6D2AE4;

	static char RankTablePath[MaxPathLength] = {};
	static char RankIconTablePath[MaxPathLength] = {};

	static uint32_t HashForName(XAssetType type, std::string_view name)
	{
		uint32_t hash = static_cast<uint32_t>(type);

		for (char raw : name)
		{
			const char c = raw >= 'A' && raw <= 'Z' ? static_cast<char>(raw + 32) : raw;
			hash = hash * 31 + static_cast<uint32_t>(c == '\\' ? '/' : c);
		}
		return hash & 0x7FFF;
	}

	static bool EqualsNoCase(std::string_view a, std::string_view b)
	{
		const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; };

		return a.size() == b.size()
			&& std::equal(a.begin(), a.end(), b.begin(), [&](char x, char y) { return lower(x) == lower(y); });
	}

	void GRank::Initialize()
	{
		if (Patch::UseCoD4X)
			return;

		NameDvar = Dvar::RegisterString("g_ranktablename", DVAR_NONE,
			"Rank table this server wants clients to read, appended to mp/rankTable and mp/rankIconTable", "");
	}

	// The name arrives in systeminfo, so it belongs to one server and has to die with the session.
	void GRank::Frame()
	{
		if (!NameDvar)
			return;

		const connstate_t state = client_ui ? client_ui->connectionState : CA_DISCONNECTED;
		if (state < CA_CONNECTED)
		{
			Clear();
			return;
		}
		Cleared = false;

		// The tables come and go with the map, so the stock literals go back for the duration of
		// every load and the lookup is redone afterwards.
		if (state < CA_ACTIVE)
		{
			Applied.clear();
			Restore();
			return;
		}

		const std::string requested = NameDvar->current.string ? NameDvar->current.string : "";
		if (requested == Applied)
			return;
		Applied = requested;

		if (requested.empty())
			Restore();
		else
			Apply(requested);
	}

	const char* GRank::Table()
	{
		return Patched ? RankTablePath : reinterpret_cast<const char*>(StockRankTable);
	}

	const char* GRank::IconTable()
	{
		return Patched ? RankIconTablePath : reinterpret_cast<const char*>(StockRankIconTable);
	}

	void GRank::Apply(const std::string& name)
	{
		if (!IsValidName(name))
		{
			Com_PrintMessage(CON_CHANNEL_FILES,
				std::format("^3This server asked for the rank table '{}', which is not a name a string table can "
							"have. Keeping the stock ranks.\n",
					name)
					.c_str(),
				0);
			Restore();
			return;
		}

		const std::string table = std::format("mp/rankTable{}.csv", name);
		const std::string icons = std::format("mp/rankIconTable{}.csv", name);

		if (!TableExists(table) || !TableExists(icons))
		{
			Com_PrintMessage(CON_CHANNEL_FILES,
				std::format("^3This server asked for the rank table '{}', which none of its fastfiles carry. "
							"Keeping the stock ranks.\n",
					name)
					.c_str(),
				0);
			Restore();
			return;
		}

		std::memcpy(RankTablePath, table.c_str(), table.size() + 1);
		std::memcpy(RankIconTablePath, icons.c_str(), icons.size() + 1);

		for (uintptr_t site : RankTableSites)
			Memory::Set<uintptr_t>(site, reinterpret_cast<uintptr_t>(RankTablePath));
		Memory::Set<uintptr_t>(RankIconTableSite, reinterpret_cast<uintptr_t>(RankIconTablePath));

		Patched = true;
		Com_PrintMessage(CON_CHANNEL_FILES, std::format("Reading ranks from {}.\n", RankTablePath).c_str(), 0);
	}

	void GRank::Restore()
	{
		if (!Patched)
			return;
		Patched = false;

		for (uintptr_t site : RankTableSites)
			Memory::Set<uintptr_t>(site, StockRankTable);
		Memory::Set<uintptr_t>(RankIconTableSite, StockRankIconTable);
	}

	// Nothing carries over between servers; CoD4x clears the dvar as the connect packet goes out
	// (cl_main.c:2896). Going through the console leaves the engine owning the string.
	void GRank::Clear()
	{
		Restore();

		if (Cleared)
			return;
		Cleared = true;

		if (Applied.empty() && (!NameDvar->current.string || !NameDvar->current.string[0]))
			return;
		Applied.clear();

		char command[] = "set g_ranktablename \"\"";
		Cmd_ExecuteSingleCommand(0, 0, command);
	}

	// A hostile server is on the other end of this. The name only ever suffixes an asset name, so
	// refuse anything else outright rather than quietly dropping characters and looking up something
	// the server never asked for.
	bool GRank::IsValidName(std::string_view name)
	{
		if (name.empty() || name.size() > MaxNameLength)
			return false;

		return std::ranges::all_of(name,
			[](char c) {
				return c == '_' || c == '-' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
					|| (c >= 'a' && c <= 'z');
			});
	}

	// Walking the entries rather than DB_FindXAssetHeader, which drops the client on a miss.
	bool GRank::TableExists(std::string_view name)
	{
		if (!db_hashTable || !g_assetEntryPool)
			return false;

		for (uint16_t index = db_hashTable[HashForName(ASSET_TYPE_STRINGTABLE, name)]; index;
			index = g_assetEntryPool[index].nextHash)
		{
			const XAssetEntry& entry = g_assetEntryPool[index];
			if (entry.asset.type != ASSET_TYPE_STRINGTABLE || !entry.asset.header.data)
				continue;

			// Every asset header opens with its own name, StringTable::name here.
			const char* found = *static_cast<const char* const*>(entry.asset.header.data);
			if (found && EqualsNoCase(found, name))
				return true;
		}
		return false;
	}
}
