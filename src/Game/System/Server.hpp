#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GServer
	{
	public:
		static gentity_s* FindPlayerEntity();
		static int GetFreeCorpseSlot();
	};
}
