#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// The demo SR-Server plays for this client, or for the player it follows, as its last state told it.
	struct ServerDemoState
	{
		int Time = 0;
		int ServerTime = 0;
		int RunTime = 0;
		float Rate = 1.0f;
		int Velocity = 0;
		int FPS = 0;
		int ForwardMove = 0;
		int RightMove = 0;
		int Buttons = 0;
		std::string Name;
	};

	class API ServerDemo
	{
	public:
		static bool Packet(const netadr_t* from, msg_t* msg);
		static const ServerDemoState* Get();
		static int RunTime();

	private:
		static inline ServerDemoState State = {};
	};
}
