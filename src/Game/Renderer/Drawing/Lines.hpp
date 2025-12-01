#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Lines : public IObject
	{
	public:
		std::vector<GfxPointVertex> Verts;
		int Vertex = 0;
		int Width = 2;
		int MaxVertex = 2000;
		bool DepthTest = true;

		Lines() = default;
		Lines(int width, int max, bool depthTest);
		virtual ~Lines() = default;

		void Add(const vec3& start, const vec3& end, const vec4& color);
		void AddBox(const vec3& position, const vec3& size, const vec4& color);
		void Render();
	};
}
