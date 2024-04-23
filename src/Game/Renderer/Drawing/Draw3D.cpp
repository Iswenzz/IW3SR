#include "Draw3D.hpp"

namespace IW3SR
{
	void GDraw3D::Box(const vec3& position, const vec3& size, const vec4& color)
	{
		BufferLines.AddBox(position, size, color);
	}

	void GDraw3D::Line(const vec3& start, const vec3& end, const vec4& color)
	{
		BufferLines.Add(start, end, color);
	}

	void GDraw3D::Render()
	{
		BufferLines.Render();
	}
}
