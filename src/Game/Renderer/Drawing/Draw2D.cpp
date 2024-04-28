#include "Draw2D.hpp"

namespace IW3SR
{
	void GDraw2D::Text(const std::string& text, Font_s* font, const vec2& position, float scale, const vec4& color)
	{
		R_AddCmdDrawText(text.c_str(), 0x7FFFFFFF, font, position.x, position.y, scale, scale, 0, 0, color);
	}

	vec2 GDraw2D::TextSize(const std::string& text, Font_s* font)
	{
		float w = static_cast<float>(R_TextWidth(text.c_str(), text.size(), font));
		float h = static_cast<float>(font->pixelHeight);
		return { w, h };
	}

	void GDraw2D::Rect(Material* material, const vec2& position, const vec2& size, const vec4& color)
	{
		R_AddCmdDrawStretchPic(material, position.x, position.y, size.x, size.y, 0.f, 0.f, 0.f, 0.f, color);
	}
}
