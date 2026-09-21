#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GCoD4X
	{
	public:
		static void Attach(HMODULE mod);
		static void Install();

		static void ReallocXAssetPools();

	private:
		static void TightenFrameLimiter();
		static void ReportMisses();
		static void WarnUntested();

		static int ReadVersion();
		static std::string FormatVersion(int version);

		static uintptr_t FindMainWndProc();
		static uintptr_t FindRestartForDemo();
		static uintptr_t FindXAssetsInitStdCount();
		static WinMouseVars_t* FindWinMouseVars();
	};
}
