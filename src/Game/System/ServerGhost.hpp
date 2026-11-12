#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// The ghost SR-Server runs beside this client, or beside the player it follows, as its last state told it.
	struct ServerGhostState
	{
		int Time = 0;
		int Slot = -1;
		int RecordTime = 0;
		int Delta = 0;
		bool HasDelta = false;
		std::string Name;
	};

	class API ServerGhost
	{
	public:
		static inline float HideDistance = 96.0f;

		static bool Packet(const netadr_t* from, msg_t* msg);
		static const ServerGhostState* Get();
		static bool SceneModel(const cpose_t* pose, int entnum);
		static bool Origin(vec3& origin);

	private:
		static inline ServerGhostState State = {};
		static inline vec3 Position = {};
		static inline int Drawn = 0;
		static inline bool Visible = false;
	};
}
