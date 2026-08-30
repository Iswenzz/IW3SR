#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class Profile
	{
	public:
		static void RegisterDvars();
		static void UseCoD4XStatsFormat();
		static char DecodeStats(saveStatData_t* data, const char* gamedir);
	};
}
