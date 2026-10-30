#include "Q3Weapons.hpp"

// Q3's MASK_SHOT is solid and bodies; CoD4's bullet mask less bodies and water, which rockets fly through
#define Q3_MASK_SHOT 0x806811
#define Q3_MASK_SOLID 0x1
#define Q3_CONTENTS_BODY 0x2000000
#define Q3_SURF_NOIMPACT (0x4 | 0x10)
#define Q3_PRESTEP_TIME 50
#define Q3_MUZZLE_OFFSET 14.0f
#define Q3_KNOCKBACK 1000.0f
#define Q3_MASS 200.0f
#define Q3_MAX_KNOCKBACK 200
#define Q3_HASTE_FACTOR 1.3
// CoD4 Rapid Fire (specialty_rof), which the mod gives for Q3 haste so pmove sees it on both ends
#define PERK_RAPID_FIRE 0x8
// CoD4 Sonic Boom (specialty_explosivedamage), which the mod gives for Q3 quad damage
#define PERK_QUAD_DAMAGE 0x1000
// The default g_quadfactor, in baseq3 and Defrag alike
#define Q3_QUAD_FACTOR 3.0f

// Q3's player box measured from CoD4's feet origin: Q3 puts its origin 24 above the feet, the box spans
// -24..32 (16 crouched) and the eye sits 26 above the origin (12 crouched)
#define Q3_HALF_WIDTH 15.0f
#define Q3_ORIGIN_HEIGHT 24.0f
// G_RadiusDamage pushes from 24 above the origin "so players get knocked into the air more"
#define Q3_KNOCKBACK_LIFT 24.0f
#define Q3_HEIGHT 56.0f
#define Q3_CROUCH_HEIGHT 40.0f
#define Q3_VIEWHEIGHT 50.0f
#define Q3_CROUCH_VIEWHEIGHT 36.0f

namespace IW3SR
{
	const std::array<Q3WeaponDef, 2> Q3Weapons::Defs = { {
		{ "gl_ak47_mp", 900.0f, 1000.0f, 1.2f, 100.0f, 120.0f, 800, 15000 },
		{ "gl_g3_mp", 2000.0f, 2000.0f, 1.0f, 15.0f, 20.0f, 100, 10000 },
	} };

	static bool Crouched(const playerState_s* ps)
	{
		return ps->pm_flags & (PMF_DUCKED | PMF_PRONE);
	}

	static void Snap(vec3& v)
	{
		for (int i = 0; i < 3; i++)
			v[i] = static_cast<float>(static_cast<int>(v[i]));
	}

	static void SnapTowards(vec3& v, const vec3& to)
	{
		for (int i = 0; i < 3; i++)
			v[i] = static_cast<float>(to[i] <= v[i] ? static_cast<int>(v[i]) : static_cast<int>(v[i]) + 1);
	}

	int Q3Weapons::Find(const char* item)
	{
		if (!item)
			return -1;

		for (int i = 0; i < static_cast<int>(Defs.size()); i++)
		{
			if (!strcmp(Defs[i].item, item))
				return i;
		}
		return -1;
	}

	// Whether PM_Weapon fired since the event sequence was at sequence.
	bool Q3Weapons::Fired(const playerState_s* ps, int sequence)
	{
		for (int i = sequence; i < ps->eventSequence; i++)
		{
			const int event = ps->events[i & 3] & 0xFF;
			if (event == EV_FIRE_WEAPON || event == EV_FIRE_WEAPON_LASTSHOT)
				return true;
		}
		return false;
	}

	// Launches a Q3 missile from the shot PM_Weapon just fired. weaponTime is the value before PM_Weapon ran:
	// Q3 carries the time fired past the refire into the next one, which CoD4 drops.
	void Q3Weapons::Fire(pmove_t* pm, pml_t* pml, Q3WeaponState& state, int weapon, int weaponTime, bool cpm,
		Q3WeaponEvents* events)
	{
		playerState_s* ps = pm->ps;
		const Q3WeaponDef& def = Defs[weapon];

		const int refire = ps->perks & PERK_RAPID_FIRE ? static_cast<int>(def.refire / Q3_HASTE_FACTOR) : def.refire;
		ps->weaponTime = refire + std::min(0, weaponTime - pml->msec);

		const float pitch = glm::radians(ps->viewangles[0]);
		const float yaw = glm::radians(ps->viewangles[1]);
		const vec3 forward = { std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), -std::sin(pitch) };

		vec3 muzzle = ps->origin;
		muzzle[2] += Crouched(ps) ? Q3_CROUCH_VIEWHEIGHT : Q3_VIEWHEIGHT;
		muzzle += forward * Q3_MUZZLE_OFFSET;
		Snap(muzzle);

		Q3Projectile projectile = {};
		projectile.weapon = weapon;
		projectile.trTime = pm->cmd.serverTime - Q3_PRESTEP_TIME;
		projectile.expireTime = pm->cmd.serverTime + def.lifetime;
		projectile.damageScale = ps->perks & PERK_QUAD_DAMAGE ? Q3_QUAD_FACTOR : 1.0f;
		projectile.trBase = muzzle;
		projectile.trDelta = forward * (cpm ? def.cpmSpeed : def.speed);
		projectile.current = muzzle;
		Snap(projectile.trDelta);

		if (state.count == Q3_MAX_PROJECTILES)
		{
			std::move(state.projectiles.begin() + 1, state.projectiles.end(), state.projectiles.begin());
			state.count--;
		}
		state.projectiles[state.count++] = projectile;

		if (events)
			events->push_back({ Q3WeaponEventType::Fire, weapon, projectile.trTime, projectile.damageScale, muzzle, forward,
				ENTITYNUM_NONE });
	}

	// Defrag CPM switches weapons with no drop or raise time; CoD4 has no such mode, so its timers are cut.
	void Q3Weapons::Switch(playerState_s* ps, bool cpm)
	{
		if (cpm && ps->weaponstate >= WEAPON_RAISING && ps->weaponstate <= WEAPON_DROPPING_QUICK)
			ps->weaponTime = 0;
	}

	// Moves every missile up to this command's time, as G_RunMissile does each server frame, and explodes the
	// ones that hit something or ran out of time. Runs before the ground trace, so a blast lands between
	// commands like in Q3.
	void Q3Weapons::Detonate(pmove_t* pm, Q3WeaponState& state, bool cpm, Q3WeaponEvents* events)
	{
		playerState_s* ps = pm->ps;

		if (ps->pm_type != PM_NORMAL)
		{
			state.count = 0;
			return;
		}
		const int time = pm->cmd.serverTime;
		const int mask = Q3_MASK_SHOT | (state.hitPlayers ? Q3_CONTENTS_BODY : 0);
		const vec3 zero = { 0, 0, 0 };
		const vec3 up = { 0, 0, 1 };
		int kept = 0;

		for (int i = 0; i < state.count; i++)
		{
			Q3Projectile& projectile = state.projectiles[i];
			const float delta = static_cast<float>(time - projectile.trTime) * 0.001f;
			const vec3 origin = projectile.trBase + projectile.trDelta * delta;

			trace_t trace = {};
			PM_PlayerTrace(pm, &trace, projectile.current, zero, zero, origin, ps->clientNum, mask);

			if (trace.startsolid || trace.allsolid)
				trace.fraction = 0.0f;
			else
				projectile.current = glm::mix(projectile.current, origin, trace.fraction);

			if (trace.fraction != 1.0f)
			{
				if (trace.surfaceFlags & Q3_SURF_NOIMPACT)
					continue;

				vec3 impact = projectile.current;
				SnapTowards(impact, projectile.trBase);
				const int hitEnt = trace.hitType == TRACE_HITTYPE_ENTITY ? trace.hitId : ENTITYNUM_NONE;
				const vec3 normal = glm::length(trace.normal) > 0.0f ? trace.normal : up;
				Explode(pm, projectile, impact, normal, hitEnt, cpm, events);
				continue;
			}
			if (time >= projectile.expireTime)
			{
				vec3 impact = origin;
				Snap(impact);
				Explode(pm, projectile, impact, up, ENTITYNUM_NONE, cpm, events);
				continue;
			}
			state.projectiles[kept++] = projectile;
		}
		state.count = kept;
	}

	// The shooter's own blast. Defrag has no self damage, and other players are pushed by the server alone.
	// The event carries the surface normal, which the impact effect faces along.
	void Q3Weapons::Explode(pmove_t* pm, const Q3Projectile& projectile, const vec3& origin, const vec3& normal,
		int hitEnt, bool cpm, Q3WeaponEvents* events)
	{
		if (events)
		{
			events->push_back({ Q3WeaponEventType::Explode, projectile.weapon, projectile.trTime, projectile.damageScale,
				origin, normal, hitEnt });
		}
		const auto visible = [pm](const vec3& from, const vec3& to)
		{
			const vec3 zero = { 0, 0, 0 };
			trace_t trace = {};
			PM_PlayerTrace(pm, &trace, from, zero, zero, to, pm->ps->clientNum, Q3_MASK_SOLID);
			return trace.fraction == 1.0f;
		};
		Splash(pm->ps, projectile.weapon, projectile.damageScale, origin, true, cpm, visible);
	}

	// G_RadiusDamage and G_Damage's knockback on one player. Returns whether it pushed them.
	bool Q3Weapons::Splash(playerState_s* ps, int weapon, float damageScale, const vec3& origin, bool self, bool cpm,
		const Q3Visible& visible)
	{
		const Q3WeaponDef& def = Defs[weapon];
		const vec3 absmin = ps->origin + vec3{ -Q3_HALF_WIDTH, -Q3_HALF_WIDTH, 0.0f };
		const vec3 absmax = ps->origin
			+ vec3{ Q3_HALF_WIDTH, Q3_HALF_WIDTH, Crouched(ps) ? Q3_CROUCH_HEIGHT : Q3_HEIGHT };

		vec3 v = { 0, 0, 0 };
		for (int i = 0; i < 3; i++)
		{
			if (origin[i] < absmin[i])
				v[i] = absmin[i] - origin[i];
			else if (origin[i] > absmax[i])
				v[i] = origin[i] - absmax[i];
		}
		const float dist = glm::length(v);
		if (dist >= def.splashRadius)
			return false;

		const float points = def.splashDamage * damageScale * (1.0f - dist / def.splashRadius);
		if (!CanDamage(origin, (absmin + absmax) * 0.5f, visible))
			return false;

		const int knockback = std::min(static_cast<int>(points), Q3_MAX_KNOCKBACK);
		if (!knockback)
			return false;

		vec3 dir = ps->origin - origin;
		dir[2] += Q3_ORIGIN_HEIGHT + Q3_KNOCKBACK_LIFT;

		const float length = glm::length(dir);
		if (length)
			dir /= length;

		// Defrag CPM pushes harder sideways on your own rocket splash
		if (cpm && self)
		{
			dir[0] *= def.cpmSelfPush;
			dir[1] *= def.cpmSelfPush;
		}
		ps->velocity += dir * (Q3_KNOCKBACK * static_cast<float>(knockback) / Q3_MASS);

		// Q3 holds the knockback a moment so walking can't cancel it straight away
		if (!ps->pm_time)
		{
			ps->pm_time = std::clamp(knockback * 2, 50, 200);
			ps->pm_flags |= PMF_TIME_KNOCKBACK;
		}
		return true;
	}

	// Q3's CanDamage: the blast reaches the box's middle or one of its four side corners.
	bool Q3Weapons::CanDamage(const vec3& origin, const vec3& midpoint, const Q3Visible& visible)
	{
		const vec3 offsets[] = {
			{ 0, 0, 0 },
			{ Q3_HALF_WIDTH, Q3_HALF_WIDTH, 0 },
			{ Q3_HALF_WIDTH, -Q3_HALF_WIDTH, 0 },
			{ -Q3_HALF_WIDTH, Q3_HALF_WIDTH, 0 },
			{ -Q3_HALF_WIDTH, -Q3_HALF_WIDTH, 0 },
		};
		for (const vec3& offset : offsets)
		{
			if (visible(origin, midpoint + offset))
				return true;
		}
		return false;
	}
}
