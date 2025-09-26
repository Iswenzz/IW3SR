#pragma once
#include "ImGUI/Drawing/Frame.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Binds frame.
	/// </summary>
	class Binds : public Frame
	{
	public:
		/// <summary>
		/// Initialize the binds frame.
		/// </summary>
		Binds();
		virtual ~Binds() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
