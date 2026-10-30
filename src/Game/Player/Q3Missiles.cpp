#include "Q3Missiles.hpp"
#include "PMove.hpp"

#define FX_SYSTEM 0x115D100
#define FX_STATUS_LOOPING 0x10000
#define Q3_PLAYED_EVENTS 128
// Impact effects sit this far off the surface hit, so their flat parts don't z-fight with it
#define Q3_IMPACT_LIFT 2.0f

namespace IW3SR
{
	// Effect and sound names per Q3Weapons::Defs entry, all shipped in the mod's fastfile. The fire sound is the
	// weapon file's, which the predicted shot already plays.
	// The plasma's trail is the bolt itself, so it goes the moment the missile does; the rocket's smoke lingers.
	struct Q3MissileLook
	{
		const char* trail;
		const char* impact;
		const char* impactSound;
		bool killTrail;
	};

	static const std::array<Q3MissileLook, 2> Looks = { {
		{ "q3/rocket_trail", "explosions/grenadeExp_default", "weap_quake_rocket_explode", false },
		{ "q3/plasma_fire", "q3/plasma_explode", "weap_quake_plasma_explode", true },
	} };

	static int64_t EventKey(int type, int weapon, int trTime)
	{
		return (static_cast<int64_t>(trTime) << 8) | (weapon << 1) | type;
	}

	static std::array<vec3, 3> AxisFromDir(const vec3& dir)
	{
		const vec3 forward = glm::length(dir) > 0.0f ? glm::normalize(dir) : vec3{ 0, 0, 1 };
		const vec3 reference = std::abs(forward.z) < 0.99f ? vec3{ 0, 0, 1 } : vec3{ 1, 0, 0 };
		const vec3 left = glm::normalize(glm::cross(reference, forward));
		return { forward, left, glm::cross(forward, left) };
	}

	static const FxEffectDef* FindEffect(const char* name)
	{
		static const auto fx_enable = Dvar::Find("fx_enable");

		if (fx_enable && !fx_enable->current.enabled)
			return nullptr;
		return static_cast<const FxEffectDef*>(DB_FindXAssetHeader_h(ASSET_TYPE_FX, name));
	}

	// FX_DelRefToEffect: the last reference lets the FX system free the effect once its elements die.
	static void ReleaseEffect(FxEffect* effect)
	{
		if (static_cast<unsigned short>(effect->status) == 1)
			FX_EffectNoLongerReferenced(reinterpret_cast<void*>(FX_SYSTEM), effect);
		InterlockedDecrement(&effect->status);
	}

	// Plays what the predicted steps fired and exploded. Prediction replays the same commands every frame,
	// so each missile's fire and impact is played once, by its fire time.
	void Q3Missiles::Events(const Q3WeaponEvents& events)
	{
		for (const Q3WeaponEvent& event : events)
		{
			const int type = event.type == Q3WeaponEventType::Fire ? 0 : 1;
			if (!Play(EventKey(type, event.weapon, event.trTime)))
				continue;

			const Q3MissileLook& look = Looks[event.weapon];
			if (event.type == Q3WeaponEventType::Fire)
			{
				Spawn(event.weapon, event.trTime, event.origin, event.dir);
				continue;
			}
			for (Q3MissileTrail& trail : Trails)
			{
				if (trail.weapon == event.weapon && trail.trTime == event.trTime)
					Stop(trail);
			}
			Effect(look.impact, event.origin + event.dir * Q3_IMPACT_LIFT, event.dir);
			CG_PlaySoundAliasByName(ENTITYNUM_WORLD, event.origin, look.impactSound);
		}
		std::erase_if(Trails, [](const Q3MissileTrail& trail) { return !trail.effect; });
	}

	// Moves every trail to its missile's place at the render time, and ends the trails of missiles the
	// prediction no longer has.
	void Q3Missiles::Frame()
	{
		if (cgs->time < LastTime)
			Reset();
		LastTime = cgs->time;

		const Q3WeaponState* state = PMove::HasQ3Weapons()
			? PMove::FindWeapons(cgs->predictedPlayerState.commandTime) : nullptr;

		for (Q3MissileTrail& trail : Trails)
		{
			const Q3Projectile* projectile = nullptr;
			for (int i = 0; state && i < state->count; i++)
			{
				const Q3Projectile& candidate = state->projectiles[i];
				if (candidate.weapon == trail.weapon && candidate.trTime == trail.trTime)
					projectile = &candidate;
			}
			if (!projectile)
			{
				Stop(trail);
				continue;
			}
			const float delta = static_cast<float>(cgs->time - projectile->trTime) * 0.001f;
			trail.effect->frameNow.origin = projectile->trBase + projectile->trDelta * delta;
		}
		std::erase_if(Trails, [](const Q3MissileTrail& trail) { return !trail.effect; });
	}

	// Forgets the trails without touching them: the FX system frees every effect on a map change.
	void Q3Missiles::Reset()
	{
		Trails.clear();
		Played.clear();
		LastTime = 0;
	}

	bool Q3Missiles::Play(int64_t key)
	{
		if (std::ranges::find(Played, key) != Played.end())
			return false;

		Played.push_back(key);
		if (Played.size() > Q3_PLAYED_EVENTS)
			Played.pop_front();
		return true;
	}

	void Q3Missiles::Spawn(int weapon, int trTime, const vec3& origin, const vec3& dir)
	{
		const FxEffectDef* def = FindEffect(Looks[weapon].trail);
		if (!def)
			return;

		const std::array<vec3, 3> axis = AxisFromDir(dir);
		FxEffect* effect = FX_SpawnOrientedEffect(def, cgs->time, origin, axis.data());
		if (effect)
			Trails.push_back({ weapon, trTime, effect });
	}

	// FX_StopEffectNonRecursive: ends the looping, which drops the reference it held, then our own. The plasma's
	// elements are removed with it, through FX_ThroughWithEffect.
	void Q3Missiles::Stop(Q3MissileTrail& trail)
	{
		if (!trail.effect)
			return;

		FxEffect* effect = trail.effect;
		trail.effect = nullptr;

		for (long status = effect->status; status & FX_STATUS_LOOPING; status = effect->status)
		{
			if (InterlockedCompareExchange(&effect->status, status & ~FX_STATUS_LOOPING, status) == status)
			{
				ReleaseEffect(effect);
				break;
			}
		}
		if (Looks[trail.weapon].killTrail)
			FX_ThroughWithEffect(effect);
		else
			ReleaseEffect(effect);
	}

	void Q3Missiles::Effect(const char* name, const vec3& origin, const vec3& dir)
	{
		const FxEffectDef* def = FindEffect(name);
		if (!def)
			return;

		const std::array<vec3, 3> axis = AxisFromDir(dir);
		if (FxEffect* effect = FX_SpawnOrientedEffect(def, cgs->time, origin, axis.data()))
			ReleaseEffect(effect);
	}
}
