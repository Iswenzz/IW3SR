#pragma once
#include "Game/Base.hpp"

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
