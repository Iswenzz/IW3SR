#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Draw current FPS.
	/// </summary>
	class FPS : public Module
	{
	public:
		int Value;
		CircularBuffer<int, 1000> Values;

		Text FrameText;
		Plots Graph;
		bool ShowGraph;

		/// <summary>
		/// Create the module.
		/// </summary>
		FPS();
		virtual ~FPS() = default;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void Menu() override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;

		SERIALIZE_POLY(FPS, Module, FrameText, Graph, ShowGraph)
	};
}
