#pragma once
#include "ImGUI/Drawing/Frame.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// About frame.
	/// </summary>
	class About : public Frame
	{
	public:
		/// <summary>
		/// Initialize the about frame.
		/// </summary>
		About();
		virtual ~About() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
