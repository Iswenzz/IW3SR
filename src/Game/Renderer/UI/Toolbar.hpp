#pragma once
#include "ImGUI/Drawing/Window.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Main toolbar.
	/// </summary>
	class Toolbar : public Window
	{
	public:
		bool IsReloading = false;
		bool IsDebug = false;

		/// <summary>
		/// Initialize the Toolbar.
		/// </summary>
		Toolbar();
		virtual ~Toolbar() = default;

		/// <summary>
		/// Reload plugins.
		/// </summary>
		void Reload();

		/// <summary>
		/// Render frame.
		/// </summary>
		void Render();

	private:
		/// <summary>
		/// Compile plugins.
		/// </summary>
		void Compile();
	};
}
