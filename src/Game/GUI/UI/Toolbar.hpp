#pragma once
#include "ImGUI/Drawing/Frame.hpp"

namespace IW3SR::UC
{
	/// <summary>
	/// Main toolbar.
	/// </summary>
	class Toolbar : public Frame
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
		/// Render frame.
		/// </summary>
		void OnRender() override;

	private:
		/// <summary>
		/// Reload plugins.
		/// </summary>
		void Reload();

		/// <summary>
		/// Compile plugins.
		/// </summary>
		void Compile();
	};
}
