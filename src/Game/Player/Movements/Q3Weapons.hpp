#pragma once
#include "CoD4.hpp"

#define Q3_MAX_PROJECTILES 64

namespace IW3SR
{
	// Quake 3's numbers for one weapon (g_missile.c, bg_pmove.c).
	struct Q3WeaponDef
	{
		const char* item;
		float speed;
		float cpmSpeed;
		float cpmSelfPush;
		float splashDamage;
		float splashRadius;
		int refire;
		int lifetime;
	};

	struct Q3Projectile
	{
		int weapon;
		int trTime;
		int expireTime;
		float damageScale;
		vec3 trBase;
		vec3 trDelta;
		vec3 current;
	};

	struct Q3WeaponState
	{
		int count = 0;
		bool hitPlayers = false;
		std::array<Q3Projectile, Q3_MAX_PROJECTILES> projectiles{};
	};

	enum class Q3WeaponEventType
	{
		Fire,
		Explode,
	};

	struct Q3WeaponEvent
	{
		Q3WeaponEventType type;
		int weapon;
		int trTime;
		float damageScale;
		vec3 origin;
		vec3 dir;
		int hitEnt;
	};

	// Whether a blast at the first point reaches the second, for Q3's CanDamage.
	using Q3Visible = std::function<bool(const vec3&, const vec3&)>;

	using Q3WeaponEvents = std::vector<Q3WeaponEvent>;

	class Q3Weapons
	{
	public:
		static const std::array<Q3WeaponDef, 2> Defs;

		static int Find(const char* item);
		static bool Fired(const playerState_s* ps, int sequence);
		static void Fire(pmove_t* pm, pml_t* pml, Q3WeaponState& state, int weapon, int weaponTime, bool cpm,
			Q3WeaponEvents* events);
		static void Switch(playerState_s* ps, bool cpm);
		static void Detonate(pmove_t* pm, Q3WeaponState& state, bool cpm, Q3WeaponEvents* events);
		static bool Splash(playerState_s* ps, int weapon, float damageScale, const vec3& origin, bool self, bool cpm,
			const Q3Visible& visible);

	private:
		static void Explode(pmove_t* pm, const Q3Projectile& projectile, const vec3& origin, const vec3& normal,
			int hitEnt, bool cpm, Q3WeaponEvents* events);
		static bool CanDamage(const vec3& origin, const vec3& midpoint, const Q3Visible& visible);
	};
}
