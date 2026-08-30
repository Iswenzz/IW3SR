#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GHuffman
	{
	public:
		static void Initialize();
		static int Decompress(const uint8_t* input, uint8_t* output, int readsize);

	private:
		static inline std::vector<uint8_t> Staging;
		static inline bool Installed = false;
		static inline bool Reported = false;
	};
}
