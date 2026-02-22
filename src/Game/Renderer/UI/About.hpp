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
		bool Checking = false;
		bool Downloading = false;
		bool Extracting = false;
		float Progress = 0.0f;
		std::string LatestVersion;
		std::string StatusMessage;

		void CheckUpdate();
		void StartUpdate();
	};
}
