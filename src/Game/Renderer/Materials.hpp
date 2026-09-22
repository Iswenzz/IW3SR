#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API GMaterials
	{
	public:
		static constexpr int MaxMaterials = 4096;
		static constexpr int MaxDrawableMaterials = 2048;

		static void Initialize();

		static int PoolSize();
		static Material** Sorted();
		static void WarnOnOverflow();

	private:
		static inline bool Relocated = false;

		static bool Verify();
		static bool Sort(Material* a, Material* b);
		static void* Clear(void* memory, int value, size_t size);
	};
}
