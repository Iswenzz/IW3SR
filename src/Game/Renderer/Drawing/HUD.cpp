#include "HUD.hpp"
#include "Draw2D.hpp"

namespace IW3SR
{
	GHUD::GHUD(const std::string& material, float x, float y, float w, float h, const vec4& color)
	{
		Position = { x, y };
		Size = { w, h };
		Color = color;
		MaterialName = material;
	}

	void GHUD::SetRectAlignment(Horizontal horizontal, Vertical vertical)
	{
		HorizontalAlign = horizontal;
		VerticalAlign = vertical;
	}

	void GHUD::SetAlignment(Alignment horizontal, Alignment vertical)
	{
		AlignX = horizontal;
		AlignY = vertical;
	}

	void GHUD::SetMaterial(const std::string& material)
	{
		Material = Material_RegisterHandle(material.c_str(), 3);
		MaterialName = material;
	}

	void GHUD::ComputeAlignment(vec2& position)
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

	void GHUD::Menu(const std::string& label, bool open)
	{
		if (!ImGui::CollapsingHeader(label, open))
			return;

		ImGui::PushID(label.c_str());

		ImGui::DragFloat2("Position", Position);
		ImGui::DragFloat2("Size", Size);
		ImGui::ColorEdit4("Color", Color, ImGuiColorEditFlags_Float);

		ImGui::ComboAlign(&AlignX, &AlignY);
		ImGui::ComboAlignRect(&HorizontalAlign, &VerticalAlign);

		ImGui::PopID();
	}

	void GHUD::Render()
	{
		if (!Material)
			SetMaterial(MaterialName);

		vec2 position = Position;
		vec2 size = Size;

		ComputeAlignment(position);
		UI::Get().Screen.Apply(position, size, HorizontalAlign, VerticalAlign);
		RenderPosition = position;
		RenderSize = size;

		GDraw2D::Rect(Material, RenderPosition, RenderSize, Color);
	}
}
