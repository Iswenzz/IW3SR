#pragma once
#include "ImGUI/Drawing/Frame.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Modules frame.
	/// </summary>
	class Modules : public Frame
	{
	public:
		/// <summary>
		/// Initialize the modules frame.
		/// </summary>
		Modules();
		virtual ~Modules() = default;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
