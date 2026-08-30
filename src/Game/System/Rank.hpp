#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// Lets a server name the rank and rank icon string tables its clients read.
	class GRank
	{
	public:
		static void Initialize();
		static void Frame();

		static const char* Table();
		static const char* IconTable();

	private:
		static void Apply(const std::string& name);
		static void Restore();
		static void Clear();

		static bool IsValidName(std::string_view name);
		static bool TableExists(std::string_view name);

		static inline dvar_s* NameDvar = nullptr;
		static inline std::string Applied;
		static inline bool Patched = false;
		static inline bool Cleared = false;
	};
}
