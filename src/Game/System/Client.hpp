#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class Client
	{
	public:
		static void Connect();
		static void Disconnect(int localClientNum);
		static void Respawn(int localClientNum);
		static void Predict(int localClientNum);
	};
}
