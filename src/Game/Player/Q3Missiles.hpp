#pragma once
#include "Game/Base.hpp"
#include "Game/Player/Movements/Q3Weapons.hpp"

namespace IW3SR
{
	// A predicted missile's trail, followed by its fire time.
	struct Q3MissileTrail
	{
		int weapon;
		int trTime;
		FxEffect* effect;
	};

	// Draws the Q3 missiles the local player predicts, so they fly and land without the server's delay.
	class API Q3Missiles
	{
	public:
		static void Events(const Q3WeaponEvents& events);
		static void Frame();
		static void Reset();

	private:
		static inline std::vector<Q3MissileTrail> Trails;
		static inline std::deque<int64_t> Played;
		static inline int LastTime = 0;

		static bool Play(int64_t key);
		static void Spawn(int weapon, int trTime, const vec3& origin, const vec3& dir);
		static void Stop(Q3MissileTrail& trail);
		static void Effect(const char* name, const vec3& origin, const vec3& dir);
	};
}
