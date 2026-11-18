#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Colors
	{
	public:
		static void Lookup(int c, uint32_t* color);
		static bool IsCode(char c);
	};
}
