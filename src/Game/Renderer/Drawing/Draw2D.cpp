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

	void GDraw2D::AngleYaw(Material* material, float start, float end, float yaw, float y, float h, const vec4& color)
	{
		const vec3 range = Math::AnglesToRange(start, end, yaw, cgs->refdef.tanHalfFovX);
		if (!range.z)
		{
			Rect(material, { range.x, y }, { range.y - range.x, h }, color);
			return;
		}
		Rect(material, { 0, y }, { range.x, h }, color);
		Rect(material, { range.y, y }, { SCREEN_WIDTH - range.y, h }, color);
	}

	void GDraw2D::LineYaw(Material* material, float angle, float yaw, float y, const vec2& size, const vec4& color)
	{
		angle = Math::AngleNormalizePI(angle - yaw);

		if (!Math::AngleInFov(angle, cgs->refdef.tanHalfFovX))
			return;

		const float x = Math::AngleScreenProjection(angle, cgs->refdef.tanHalfFovX);
		Rect(material, { x - size.x / 2, y }, size, color);
	}
}
