#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Assets
	{
	public:
		static void Initialize();

		static void LoadXAssets(XZoneInfo* zoneInfo, unsigned int zoneCount, int sync);
		static bool LoadImageFromFile(GfxImage* image, void* reader);
		static void* FindXAssetHeader(int type, const char* name);
		static void RegisterItems();
		static void RegisterWeapons(const char** weapons, int weaponCount);
		static bool ZoneExists(const std::string& name);
		static bool IsReserved(const std::string& name);

	private:
		static inline dvar_s* IgnoreMissingZones = nullptr;
		static inline dvar_s* HideMissingFx = nullptr;
		static inline dvar_s* SubstituteViewmodel = nullptr;

		static void* SubstituteViewhands(void* header, const char* name);

		static std::string ModZonePath();
	};
}
