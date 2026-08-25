#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	class FPS : public Module
	{
	public:
		int Switch = 0;
		int Frames = 0;

		Text SwitchText;
		Text FramesText;

		bool ShowSwitch;
		bool ShowFrames;

		FPS();
		virtual ~FPS() = default;

		void Menu() override;
		void OnRender() override;

	private:
		float Elapsed = 0;
		int Counted = 0;

		void Measure();
		void DrawTimestep();
		vec4 FrameColor() const;

		SERIALIZE_POLY(FPS, Module, SwitchText, FramesText, ShowSwitch, ShowFrames)
	};
}
