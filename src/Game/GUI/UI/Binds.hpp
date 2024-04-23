#pragma once
#include "ImGUI/Drawing/Window.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Binds window.
	/// </summary>
	class Binds : public Window
	{
	public:
		/// <summary>
		/// Initialize the binds window.
		/// </summary>
		Binds();
		virtual ~Binds() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void Render();
	};
}
