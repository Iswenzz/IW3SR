#pragma once
#include "ImGUI/Drawing/Window.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Settings window.
	/// </summary>
	class Settings : public Window
	{
	public:
		/// <summary>
		/// Initialize the settings window.
		/// </summary>
		Settings();
		virtual ~Settings() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
