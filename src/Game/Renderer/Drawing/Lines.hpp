#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Lines class.
	/// </summary>
	class API Lines : public IObject
	{
	public:
		std::vector<GfxPointVertex> Verts;
		int Vertex = 0;
		int Width = 2;
		int MaxVertex = 2000;
		bool DepthTest = true;

		/// <summary>
		/// Initialize lines.
		/// </summary>
		Lines() = default;

		/// <summary>
		/// Initialize lines.
		/// </summary>
		/// <param name="width">The lines width.</param>
		/// <param name="max">The max vertex size.</param>
		/// <param name="depthTest">The depth test.</param>
		Lines(int width, int max, bool depthTest);
		virtual ~Lines() = default;

		/// <summary>
		/// Add a line.
		/// </summary>
		/// <param name="start">The start point.</param>
		/// <param name="end">The end point.</param>
		/// <param name="color">The color.</param>
		void Add(const vec3& start, const vec3& end, const vec4& color);

		// <summary>
		/// Add a box.
		/// </summary>
		/// <param name="position">The position.</param>
		/// <param name="size">The size.</param>
		/// <param name="color">The color.</param>
		void AddBox(const vec3& position, const vec3& size, const vec4& color);

		/// <summary>
		/// Draw frame.
		/// </summary>
		void Render();
	};
}
