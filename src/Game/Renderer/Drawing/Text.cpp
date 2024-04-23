#include "Text.hpp"
#include "Draw2D.hpp"

namespace IW3SR
{
	GText::GText(const std::string& text, const std::string& font, float x, float y, float size, const vec4& color)
	{
		Value = text;
		Position = { x, y };
		Color = color;
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
		Font = R_RegisterFont(font.c_str(), font.size());
		FontName = font;
		FontIndex = std::distance(GDraw2D::FontNames.begin(), std::ranges::find(GDraw2D::FontNames, FontName));
	}

	void GText::ComputeAlignment(vec2& position)
	{
		if (AlignX & ALIGN_CENTER)
			position.x += -(Size.x / 2.f);
		else if (AlignX & ALIGN_RIGHT)
			position.x += -Size.x;

		if (AlignY & ALIGN_MIDDLE)
			position.y += Size.y / 2.f;
		else if (AlignY & ALIGN_BOTTOM)
			position.y += Size.y;
	}

	void GText::Menu(const std::string& label, bool open)
	{
		if (!ImGui::CollapsingHeader(label, open))
			return;

		ImGui::PushID(label.c_str());

		ImGui::DragFloat2("Position", Position);
		ImGui::ColorEdit4("Color", Color, ImGuiColorEditFlags_Float);

		if (ImGui::InputFloat("Font Size", &FontSize, 0.1))
			SetFont(FontName);

		const auto& fonts = GDraw2D::FontNames;
		if (ImGui::Combo("Font", &FontIndex, fonts))
			SetFont(fonts[FontIndex]);

		ImGui::ComboAlign(&AlignX, &AlignY);
		ImGui::ComboAlignRect(&HorizontalAlign, &VerticalAlign);

		ImGui::PopID();
	}

	void GText::Render()
	{
		if (!Font)
			SetFont(FontName);

		RenderSize = GDraw2D::TextSize(Value, Font) * FontSize;
		Size = UI::Get().Screen.RealToVirtual * RenderSize;

		vec2 position = Position;
		vec2 size = Size;

		ComputeAlignment(position);
		UI::Get().Screen.Apply(position, HorizontalAlign, VerticalAlign);
		RenderPosition = position;

		GDraw2D::Text(Value, Font, RenderPosition, FontSize, Color);
	}
}
