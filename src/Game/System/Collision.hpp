#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API GCollision
	{
	public:
		static void Initialize();
		static void InitThreadData(unsigned int threadContext);
		static const float* Verts(const uint16_t* indices);
		static const float* Vertex(const uint16_t* indices, int corner);

	private:
		static inline std::vector<uint16_t> Windows;

		static void Build();
	};
}
