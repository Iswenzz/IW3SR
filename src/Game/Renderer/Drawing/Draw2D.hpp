#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Draw 2D class.
	/// </summary>
	class API GDraw2D
	{
	public:
		static inline std::vector<std::string> FontNames = { FONT_OBJECTIVE, FONT_NORMAL, FONT_CONSOLE, FONT_SMALL,
			FONT_SMALL_DEV, FONT_BIG, FONT_BIG_DEV, FONT_BOLD };

		/// <summary>
		/// Draw text.
		/// </summary>
		/// <param name="text">The text.</param>
		/// <param name="font">The font.</param>
		/// <param name="position">The position.</param>
		/// <param name="scale">The font scale.</param>
		/// <param name="color">The color.</param>
		static void Text(const std::string& text, Font_s* font, const vec2& position, float size, const vec4& color);

		/// <summary>
		/// Get the text size.
		/// </summary>
		/// <param name="text">The text.</param>
		/// <param name="font">The font.</param>
		/// <returns></returns>
		static vec2 TextSize(const std::string& text, Font_s* font);

		/// <summary>
		/// Draw a rectangle with the specified color.
		/// </summary>
		/// <param name="material">The material.</param>
		/// <param name="position">The position.</param>
		/// <param name="size">The size.</param>
		/// <param name="color">The rect color.</param>
		static void Rect(Material* material, const vec2& position, const vec2& size, const vec4& color);

		/// <summary>
		/// Draw a section of a circle with a specified yaw angle, starting and ending angles, and color.
		/// </summary>
		/// <param name="material">The material.</param>
		/// <param name="start">Starting angle in radians.</param>
		/// <param name="end">Ending angle in radians.</param>
		/// <param name="yaw">Yaw angle in radians.</param>
		/// <param name="y">Y-coordinate of the circle's center.</param>
		/// <param name="h">Radius of the circle.</param>
		/// <param name="color">The color.</param>
		static void AngleYaw(Material* material, float start, float end, float yaw, float y, float h,
			const vec4& color);

		/// <summary>
		/// Draw a line at a specified yaw angle with a given width, height, and color.
		/// </summary>
		/// <param name="material">The material.</param>
		/// <param name="angle">Angle at which to draw the line in radians.</param>
		/// <param name="yaw">Yaw angle in radians.</param>
		/// <param name="y">Y-coordinate of the line's position.</param>
		/// <param name="size">The size.</param>
		/// <param name="color">The color.</param>
		static void LineYaw(Material* material, float angle, float yaw, float y, const vec2& size, const vec4& color);
	};
}
