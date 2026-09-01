#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	// The About page and the update check behind it. Lives outside of any one frame because the
	// HUD reads UpdateAvailable long before the menu is ever opened.
	class About
	{
	public:
		static inline bool UpdateAvailable = false;

		static void Initialize();
		static void Render();

	private:
		static inline Ref<Texture> Logo = nullptr;

		static inline bool Checking = false;
		static inline bool Downloading = false;
		static inline bool Extracting = false;
		static inline std::atomic<float> Progress = 0.0f;
		static inline std::string LatestVersion;
		static inline std::string StatusMessage;

		static void CheckUpdate();
		static void StartUpdate();
	};
}
