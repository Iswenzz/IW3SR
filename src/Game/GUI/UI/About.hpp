#pragma once
#include "ImGUI/Drawing/Window.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// About window.
	/// </summary>
	class About : public Window
	{
	public:
		/// <summary>
		/// Initialize the about window.
		/// </summary>
		About();
		virtual ~About() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
