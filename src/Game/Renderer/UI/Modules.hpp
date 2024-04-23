#pragma once
#include "ImGUI/Drawing/Window.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Modules window.
	/// </summary>
	class Modules : public Window
	{
	public:
		/// <summary>
		/// Initialize the modules.
		/// </summary>
		Modules();
		virtual ~Modules() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void Render();
	};
}
