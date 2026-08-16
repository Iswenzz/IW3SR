#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class About : public Frame
	{
	public:
		static inline bool UpdateAvailable = false;

		About();
		virtual ~About() = default;
		void OnRender() override;

	private:
		Ref<Texture> Logo = nullptr;

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
