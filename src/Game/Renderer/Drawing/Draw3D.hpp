#pragma once
#include "Game/Base.hpp"

#include "Lines.hpp"

namespace IW3SR
{
	class API GDraw3D
	{
	public:
		static inline Lines BufferLines = Lines(1, 2000, true);

		static void Box(const vec3& position, const vec3& size, const vec4& color);
		static void Line(const vec3& start, const vec3& end, const vec4& color);
		static void Render();
	};
}
