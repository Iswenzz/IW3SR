#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class Patch
	{
	public:
		static inline bool UseBase;
		static inline bool UseCoD4X;
		static inline bool AllowCoD4X = true;

		static void Initialize();
		static void Base();
		static void CoD4X(HMODULE mod);

		static void ReallocXAssetPools();
		static void ReallocXAssetPoolsX();

		static void FrameWait();

	private:
		static void CoD4X_21_3();
		static void WarnUnsupportedCoD4X();

		static void DisablePunkbuster();
		static void FixDownloadRate();
		static void TightenFrameLimiter();
		static void TightenFrameLimiterX();
		static void SkipImproperQuitPrompt();
		static void SkipOptimalSettingsPrompt();
		static void WidenColorEscapes();
		static void RenameConsolePrompt();
		static void RecolorConsoleText();

		static int GetCoD4XVersion();
	};
}
