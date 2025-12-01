#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API GHUD : public IObject
	{
	public:
		vec2 Position;
		vec2 Size;
		vec2 RenderPosition;
		vec2 RenderSize;
		vec4 Color;

		Horizontal HorizontalAlign = Horizontal::Left;
		Vertical VerticalAlign = Vertical::Top;
		Alignment AlignX = Alignment::Left;
		Alignment AlignY = Alignment::Top;
		std::string MaterialName;

		GHUD() = default;
		GHUD(const std::string& material, float x, float y, float w, float h, const vec4& color);
		virtual ~GHUD() = default;

		void SetRectAlignment(Horizontal horizontal, Vertical vertical);
		void SetAlignment(Alignment horizontal, Alignment vertical);
		void SetMaterial(const std::string& material);

		void Menu(const std::string& label, bool open = false);
		void Render();

	private:
		Material* Material = nullptr;

		void ComputeAlignment(vec2& position);

		SERIALIZE_POLY_BASE(GHUD, Position, Size, Color, HorizontalAlign, VerticalAlign, AlignX, AlignY, MaterialName)
	};
}
