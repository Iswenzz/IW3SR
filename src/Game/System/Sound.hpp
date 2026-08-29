#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GSound
	{
	public:
		static bool Command(const std::string& command);
		static void Pause();
		static void Unpause();
		static void StopAmbient(int fadeTime);
	};
}
