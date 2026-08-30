#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GPMem
	{
	public:
		static constexpr int MinMegs = 128;
		static constexpr int MaxMegs = 600;
		static constexpr int DefaultMegs = 512;

		static void Initialize();
		static void InitDvars();

		static int Reserved();

	private:
		static void Apply(int megs);

		static inline dvar_s* MegsDvar = nullptr;
		static inline int Megs = 0;
	};
}
