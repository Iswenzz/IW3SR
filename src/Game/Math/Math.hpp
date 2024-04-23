#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Math class.
	/// </summary>
	class API GMath
	{
	public:
		/// <summary>
		/// Converts a 3D position to screen space.
		/// </summary>
		/// <param name="position">The 3D position.</param>
		/// <returns></returns>
		static vec2 WorldToScreen(const vec3& position);
	};
}
