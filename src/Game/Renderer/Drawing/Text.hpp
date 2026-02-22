#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API GText : public IObject
	{
	public:
		std::string Value;
		vec2 Position;
		vec2 Size;
		vec2 RenderPosition;
		vec2 RenderSize;
		vec4 Color;

		Horizontal HorizontalAlign = Horizontal::Left;
		Vertical VerticalAlign = Vertical::Top;
		Alignment AlignX = Alignment::Left;
		Alignment AlignY = Alignment::Top;

		std::string FontName;
		float FontRescale = 0.4f;
		float FontSize = 1.4;
		int FontIndex = 0;
		bool FontResponsive = true;

		GText() = default;
		GText(const std::string& text, const std::string& font, float x, float y, float size, const vec4& color);
		virtual ~GText() = default;

		void SetRectAlignment(Horizontal horizontal, Vertical vertical);
		void SetAlignment(Alignment horizontal, Alignment vertical);
		void SetFont(const std::string& font);
		void SetResponsiveFont();

		void Menu(const std::string& label, bool open = false);
		void Render();

	private:
		Font_s* Font = nullptr;

		void ComputeAlignment(vec2& position);

		SERIALIZE_POLY_BASE(GText, Value, Position, Color, HorizontalAlign, VerticalAlign, AlignX, AlignY, FontName,
			FontSize)
	};
}
