#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	class FPS : public Module
	{
	public:
		int Value;
		CircularBuffer<int, 1000> Values;

		Text FrameText;
		Plots Graph;
		bool ShowGraph;

		FPS();
		virtual ~FPS() = default;

		void Menu() override;
		void OnRender() override;

		SERIALIZE_POLY(FPS, Module, FrameText, Graph, ShowGraph)
	};
}
