#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Text element.
	/// </summary>
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

		/// <summary>
		/// Create a text.
		/// </summary>
		GText() = default;
		virtual ~GText() = default;

		/// <summary>
		/// Create a text.
		/// </summary>
		/// <param name="text">The text.</param>
		/// <param name="font">The font.</param>
		/// <param name="x">X value.</param>
		/// <param name="y">Y value.</param>
		/// <param name="size">Font size.</param>
		/// <param name="color">The color.</param>
		GText(const std::string& text, const std::string& font, float x, float y, float size, const vec4& color);

		/// <summary>
		/// Set the rect alignment.
		/// </summary>
		/// <param name="horizontal">Horizontal aligment.</param>
		/// <param name="vertical">Vertical alignment.</param>
		void SetRectAlignment(Horizontal horizontal, Vertical vertical);

		/// <summary>
		/// Set the element alignment.
		/// </summary>
		/// <param name="horizontal">Horizontal aligment.</param>
		/// <param name="vertical">Vertical aligment.</param>
		void SetAlignment(Alignment horizontal, Alignment vertical);

		/// <summary>
		/// Set/update font.
		/// </summary>
		void SetFont(const std::string& font);

		/// <summary>
		/// Render menu.
		/// </summary>
		/// <param name="label">The label.</param>
		/// <param name="open">Default open.</param>
		void Menu(const std::string& label, bool open = false);

		/// <summary>
		/// Render text.
		/// </summary>
		void Render();

	private:
		Font_s* Font = nullptr;

		/// <summary>
		/// Compute the alignment.
		/// </summary>
		/// <param name="position">The position.</param>
		void ComputeAlignment(vec2& position);

		SERIALIZE_POLY_BASE(GText, Value, Position, Color, HorizontalAlign, VerticalAlign, AlignX, AlignY, FontName,
			FontSize)
	};
}
