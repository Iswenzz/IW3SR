#include "Text.hpp"
#include "Draw2D.hpp"

namespace IW3SR
{
	GText::GText(const std::string& text, const std::string& font, float x, float y, float size, const vec4& color)
	{
		Value = text;
		Position = { x, y };
		Color = color;
		FontSize = size;
		FontName = font;
	}

	void GText::SetRectAlignment(Horizontal horizontal, Vertical vertical)
	{
		HorizontalAlign = horizontal;
		VerticalAlign = vertical;
	}

	void GText::SetAlignment(Alignment horizontal, Alignment vertical)
	{
		AlignX = horizontal;
		AlignY = vertical;
	}

	void GText::SetFont(const std::string& font)
	{
		Font = R_RegisterFont(font.c_str(), FONT_IMAGE_TRACK);
		FontName = font;

		const auto it = std::ranges::find(GDraw2D::FontNames, FontName);
		FontIndex =
			it != GDraw2D::FontNames.end() ? static_cast<int>(std::distance(GDraw2D::FontNames.begin(), it)) : 0;
	}

	void GText::SetResponsiveFont()
	{
		if (UI::Size >= 4)
			SetFont(FONT_EXTRA_BIG);
		else if (UI::Size >= 3)
			SetFont(FONT_BIG);
		else if (UI::Size >= 2)
			SetFont(FONT_NORMAL);
		else
			SetFont(FONT_SMALL);
	}

	void GText::ComputeAlignment(vec2& position)
	{
		if (AlignX == Alignment::Center)
			position.x += -(Size.x / 2.f);
		else if (AlignX == Alignment::Right)
			position.x += -Size.x;

		if (AlignY == Alignment::Middle)
			position.y += Size.y / 2.f;
		else if (AlignY == Alignment::Bottom)
			position.y += Size.y;
	}

	void GText::Menu(const std::string& label, bool open)
	{
		if (!ImGui::BeginSection(label, open))
			return;

		ImGui::Property("Position");
		ImGui::DragFloat2("##position", &Position.x);
		ImGui::Property("Color");
		ImGui::ColorEdit4("##color", &Color.x, ImGuiColorEditFlags_Float);

		ImGui::Property("Font Size");
		if (ImGui::InputFloat("##fontsize", &FontSize, 0.1))
			SetFont(FontName);

		ImGui::Property("Responsive Font");
		ImGui::Switch("##responsive", &FontResponsive);

		const auto& fonts = GDraw2D::FontNames;
		ImGui::Property("Font");
		if (ImGui::Combo("##font", &FontIndex, fonts))
		{
			FontResponsive = false;
			SetFont(fonts[FontIndex]);
		}

		ImGui::ComboAlign(&AlignX, &AlignY);
		ImGui::ComboAlignRect(&HorizontalAlign, &VerticalAlign);

		ImGui::EndSection();
	}

	void GText::Render()
	{
		if (!Font)
			SetFont(FontName);

		if (FontResponsive)
			SetResponsiveFont();

		RenderSize = GDraw2D::TextSize(Value, Font) * FontSize;
		Size = UI::Screen.RealToVirtual * RenderSize;

		vec2 position = Position;
		vec2 size = Size;

		ComputeAlignment(position);
		UI::Screen.Apply(position, HorizontalAlign, VerticalAlign);
		RenderPosition = position;

		GDraw2D::Text(Value, Font, RenderPosition, FontSize, Color);
	}
}
