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
		static inline bool UseStandalone;

		static void Initialize();
		static void Base();
		static void RestoreWinMain();

		static void FrameWait();
		static void ReallocXAssetPools();

	private:
		static int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int show);
		static void Standalone();

		static void AllowMultipleInstances();
		static void DisableAimAssistTargets();
		static void DisablePunkbuster();
		static void FixDownloadRate();
		static void TightenFrameLimiter();
		static void SkipImproperQuitPrompt();
		static void SkipOptimalSettingsPrompt();
		static void WidenColorEscapes();
		static void RenameConsolePrompt();
		static void RecolorConsoleText();
	};
}
