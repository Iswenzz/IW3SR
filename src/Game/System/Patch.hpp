#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class Patch
	{
	public:
		static inline bool UseBase;
		static inline bool UseCoD4X;

		static void Initialize();
		static void Base();
		static void CoD4X(HMODULE mod);

		static void ReallocXAssetPools();
		static void ReallocXAssetPoolsX();

	private:
		static void CoD4X_21_1();
		static void CoD4X_21_3();

		static int GetCoD4XVersion();
	};
}
