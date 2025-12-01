#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API GDraw2D
	{
	public:
		static inline std::vector<std::string> FontNames = { FONT_OBJECTIVE, FONT_NORMAL, FONT_CONSOLE, FONT_SMALL,
			FONT_SMALL_DEV, FONT_BIG, FONT_BIG_DEV, FONT_BOLD };

		static void Text(const std::string& text, Font_s* font, const vec2& position, float size, const vec4& color);
		static vec2 TextSize(const std::string& text, Font_s* font);
		static void Rect(Material* material, const vec2& position, const vec2& size, const vec4& color);
	};
}
