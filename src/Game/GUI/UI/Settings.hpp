#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Settings frame.
	/// </summary>
	class Settings : public Frame
	{
	public:
		/// <summary>
		/// Initialize the settings frame.
		/// </summary>
		Settings();
		virtual ~Settings() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
