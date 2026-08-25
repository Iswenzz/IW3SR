#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Assets
	{
	public:
		static inline std::vector<std::string> Skipped;

		static void Initialize();

		static void LoadXAssets(XZoneInfo* zoneInfo, unsigned int zoneCount, int sync);
		static bool ZoneExists(const std::string& name);
		static bool IsReserved(const std::string& name);

		static void Reset();

	private:
		static inline dvar_s* IgnoreMissingZones = nullptr;

		static std::string ModZonePath();
	};
}
