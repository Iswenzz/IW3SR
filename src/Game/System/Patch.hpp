#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	enum class CoD4XVersion
	{
		V21_1,
		V21_3,
	};

	class Patch
	{
	public:
		static inline bool UseBase;
		static inline bool UseCoD4X;
		static inline CoD4XVersion CoD4XVer;

		static void Initialize();
		static void Base();
		static void CoD4X(HMODULE mod);

		static void ReallocXAssetPools();
		static void ReallocXAssetPoolsX();

	private:
		static CoD4XVersion DetectCoD4XVersion();
		static void CoD4X_21_1();
		static void CoD4X_21_3();
		static void WriteXAssetStdCounts();
	};
}
