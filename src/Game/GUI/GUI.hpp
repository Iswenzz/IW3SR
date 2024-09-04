#pragma once
#include "Game/Base.hpp"

#include "Core/Input/Keyboard.hpp"

namespace IW3SR
{
	/// <summary>
	/// GUI class.
	/// </summary>
	class API GUI
	{
	public:
		static inline Keyboard KeyOpen;

		/// <summary>
		/// Initialize GUI.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Frame GUI.
		/// </summary>
		static void Frame();
	};
}
