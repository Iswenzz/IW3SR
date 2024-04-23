#pragma once
#include "Game/Base.hpp"
#include "Lines.hpp"

namespace IW3SR
{
	/// <summary>
	/// Draw 3D class.
	/// </summary>
	class API GDraw3D
	{
	public:
		static inline Lines BufferLines = Lines(1, 2000, true);

		// <summary>
		/// Draw a box.
		/// </summary>
		/// <param name="position">The position.</param>
		/// <param name="size">The size.</param>
		/// <param name="color">The color.</param>
		static void Box(const vec3& position, const vec3& size, const vec4& color);

		/// <summary>
		/// Draw a line.
		/// </summary>
		/// <param name="start">The start point.</param>
		/// <param name="end">The end point.</param>
		/// <param name="color">The color.</param>
		static void Line(const vec3& start, const vec3& end, const vec4& color);

		/// <summary>
		/// Draw frame.
		/// </summary>
		static void Render();
	};
}
