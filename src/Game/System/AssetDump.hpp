#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	struct AssetRecord
	{
		XAssetType Type;
		std::string Name;
		int Zone;
		XAssetHeader Header;
	};

	class GAssetDump
	{
	public:
		static void Initialize();
		static bool Command(const std::string& command);

	private:
		static inline dvar_s* Developer = nullptr;
		static inline dvar_s* Archive = nullptr;

		static bool Allowed();
		static std::optional<XAssetType> Parse(const std::string& name);

		static void Walk(const std::function<void(const XAssetEntry&)>& visit);
		static std::vector<AssetRecord> Collect(std::optional<XAssetType> type, const std::string& filter);
		static std::array<int, ASSET_TYPE_COUNT> Count();
		static void Take(std::vector<AssetRecord>& records, const XAssetEntry& entry, std::optional<XAssetType> type,
			const std::string& filter);

		static void List(std::optional<XAssetType> type, const std::string& filter);
		static void Usage();
		static void Dump(std::optional<XAssetType> type, const std::string& filter);

		static bool Index(const std::filesystem::path& root, const std::vector<AssetRecord>& records);
		static bool Write(const std::filesystem::path& root, const AssetRecord& record);

		static const char* Name(const XAsset& asset);
		static std::string_view TypeName(XAssetType type);
		static std::string ZoneName(int zone);
		static std::string Safe(const std::string& name);
	};
}
