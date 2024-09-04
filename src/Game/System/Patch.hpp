#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Game patch.
	/// </summary>
	class Patch
	{
	public:
		/// <summary>
		/// Initialize patch.
		/// </summary>
		static void Initialize();

	private:
		/// <summary>
		/// Patch definitions.
		/// </summary>
		static void Definitions();

		/// <summary>
		/// Renderer patch.
		/// </summary>
		static void Renderer();

		/// <summary>
		/// Database patch.
		/// </summary>
		static void Database();

		/// <summary>
		/// Player patch.
		/// </summary>
		static void Player();

		/// <summary>
		/// System patch.
		/// </summary>
		static void System();

		/// <summary>
		/// CoD4X client modifications.
		/// </summary>
		static void CoD4X();

		/// <summary>
		/// Hook application.
		/// </summary>
		static void Hook();
	};
}
