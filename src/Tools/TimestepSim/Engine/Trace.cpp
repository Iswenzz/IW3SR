#include "Engine/Trace.hpp"
#include "Engine/Pmove.hpp"
#include "Game/Base.hpp"

#include <algorithm>

// Defined at global scope by src/Game/Player/Movements/CoD4.cpp, which the simulator compiles in.
void BG_AddPredictableEventToPlayerstate(int event, int parms, playerState_s* ps);

namespace
{
	using namespace IW3SR;

	constexpr int CONTENTS_SOLID = 0x1;
	constexpr int ENTITYNUM_WORLD = 1022;
	constexpr int MAXTOUCH = 32;

	// Q3 surface bit numbering, which CoD4 kept. Only SURF_SLICK is named in Types.hpp; these two
	// are the other bits PM_CrashLand branches on, read off the raw masks in bg_pmove.cpp.
	constexpr int SURF_NODAMAGE = 0x1;
	constexpr int SURF_NOSTEPS = 0x2000;
	constexpr int SURF_TYPE_MASK = 0x1F00000;
	constexpr int SURF_TYPE_SHIFT = 20;

	constexpr float WALKABLE_NORMAL_Z = 0.699999988079071f;

	// A box that comes to rest exactly on a surface must not read as solid, and the landing lerp
	// only reaches the surface to within a few ULPs. Anything shallower than this counts as
	// touching rather than penetrating. The engine gets the same slack for free by backing every
	// contact off 0.125 units along the hit normal, which this trace deliberately does not do.
	constexpr float SOLID_EPSILON = 1.0f / 1024.0f;

	// bg_pmove.cpp CorrectSolidDeltas. The order is the search order: the first free point wins.
	const vec3 CorrectSolidDeltas[26] =
	{
		{ 0.0f, 0.0f, 1.0f },
		{ -1.0f, 0.0f, 1.0f },
		{ 0.0f, -1.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f },
		{ -1.0f, 0.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ -1.0f, 0.0f, -1.0f },
		{ 0.0f, -1.0f, -1.0f },
		{ 1.0f, 0.0f, -1.0f },
		{ 0.0f, 1.0f, -1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ -1.0f, -1.0f, 0.0f },
		{ 1.0f, -1.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ -1.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f }
	};

	struct BrushHit
	{
		float fraction = 1.0f;
		vec3 normal = vec3(0.0f);
		bool hit = false;
		bool startsolid = false;
		bool allsolid = false;
	};

	// Swept AABB against one brush, expanded into the Minkowski sum of the brush and the player
	// box. The ray is the player origin, which sits on the box's bottom face rather than at its
	// centre, so the expansion is asymmetric in z: emaxs[2] moves by -mins[2], which is zero.
	// Plane order and the enter/leave bookkeeping follow CM_TraceThroughBrush in cm_trace.cpp.
	BrushHit TraceBrush(const Sim::Brush& brush, const vec3& start, const vec3& end,
		const vec3& mins, const vec3& maxs, float best)
	{
		const vec3 emins = brush.mins - maxs;
		const vec3 emaxs = brush.maxs - mins;

		float enter = 0.0f;
		float leave = best;
		bool leadside = false;
		bool allsolid = true;
		vec3 normal(0.0f);
		BrushHit out;

		for (int side = 0; side < 6; side++)
		{
			const int axis = side % 3;
			const float sign = side < 3 ? -1.0f : 1.0f;
			const float plane = side < 3 ? emins[axis] : emaxs[axis];
			const float d1 = (start[axis] - plane) * sign;
			const float d2 = (end[axis] - plane) * sign;

			if (d1 < -SOLID_EPSILON)
			{
				if (d2 <= 0.0f)
					continue;

				const float frac = d1 / (d1 - d2);
				if (frac <= enter)
					return out;

				allsolid = false;
				leave = std::min(leave, frac);
			}
			else
			{
				if (d1 <= d2)
					return out;
				if (d2 > 0.0f)
					allsolid = false;

				const float frac = std::max(d1 / (d1 - d2), 0.0f);
				if (frac > enter)
				{
					enter = frac;
					if (leave <= enter)
						return out;
				}
				else if (leadside)
					continue;

				normal = vec3(0.0f);
				normal[axis] = sign;
				leadside = true;
			}
		}

		if (leadside)
		{
			out.fraction = enter;
			out.normal = normal;
			out.hit = true;
		}
		else
		{
			out.startsolid = true;
			out.allsolid = allsolid;
			if (allsolid)
				out.fraction = 0.0f;
		}
		return out;
	}

	// bg_pmove.cpp PM_GroundSurfaceType. Zero means the landing is silent and fires no event.
	int GroundSurfaceType(const pml_t* pml)
	{
		if (pml->groundTrace.surfaceFlags & SURF_NOSTEPS)
			return 0;
		return (pml->groundTrace.surfaceFlags & SURF_TYPE_MASK) >> SURF_TYPE_SHIFT;
	}

	int LightLandingForSurface(const pml_t* pml) { return GroundSurfaceType(pml) ? 74 : 0; }
	int MediumLandingForSurface(const pml_t* pml) { return GroundSurfaceType(pml) ? 73 : 0; }

	int HardLandingForSurface(const pml_t* pml)
	{
		const int type = GroundSurfaceType(pml);
		return type ? type + 77 : 0;
	}

	int DamageLandingForSurface(const pml_t* pml)
	{
		const int type = GroundSurfaceType(pml);
		return type ? type + 106 : 0;
	}

	// The engine's BG_AddPredictableEventToPlayerstate drops event 0; the simulator's copy in
	// Movements/CoD4.cpp does not, so the check lives here to keep eventSequence on vanilla's count.
	void AddEvent(playerState_s* ps, int event, int parm)
	{
		if (event)
			BG_AddPredictableEventToPlayerstate(event, parm, ps);
	}
}

namespace Sim
{
	static const World* g_world = nullptr;

	World World::Flat()
	{
		World world;
		world.brushes.push_back({ vec3(-100000.0f, -100000.0f, -16384.0f), vec3(100000.0f, 100000.0f, 0.0f), 0 });
		return world;
	}

	void SetWorld(const World* world)
	{
		g_world = world;
	}

	const World* GetWorld()
	{
		return g_world;
	}
}

namespace IW3SR
{
	void PM_PlayerTrace(pmove_t* pm, trace_t* results, const vec3& start, const vec3& mins, const vec3& maxs,
		const vec3& end, int pass_entity_num, int content_mask)
	{
		*results = {};
		results->fraction = 1.0f;
		results->hitType = TRACE_HITTYPE_NONE;

		const Sim::World* world = Sim::GetWorld();
		if (!world || !(content_mask & CONTENTS_SOLID))
			return;

		for (const Sim::Brush& brush : world->brushes)
		{
			const BrushHit hit = TraceBrush(brush, start, end, mins, maxs, results->fraction);
			if (hit.startsolid)
			{
				results->startsolid = true;
				results->contents = CONTENTS_SOLID;
				if (hit.allsolid)
				{
					results->allsolid = true;
					results->fraction = 0.0f;
					results->surfaceFlags = 0;
				}
				continue;
			}
			if (!hit.hit || hit.fraction >= results->fraction)
				continue;

			results->fraction = hit.fraction;
			results->normal = hit.normal;
			results->surfaceFlags = brush.surfaceFlags;
			results->contents = CONTENTS_SOLID;
		}

		// CG_Trace and SV_Trace relabel a world hit as an entity hit on ENTITYNUM_WORLD, which is
		// what PM_GroundTrace reads back to decide it is standing on something.
		if (results->fraction < 1.0f)
		{
			results->hitType = TRACE_HITTYPE_ENTITY;
			results->hitId = ENTITYNUM_WORLD;
		}
		if (!results->startsolid)
			results->walkable = results->normal[2] >= WALKABLE_NORMAL_Z;
	}

	bool PM_CorrectAllSolid(pmove_t* pm, pml_t* pml, trace_t* trace)
	{
		playerState_s* ps = pm->ps;

		for (int i = 0; i < 26; i++)
		{
			vec3 point = ps->origin + CorrectSolidDeltas[i];
			PM_PlayerTrace(pm, trace, point, pm->mins, pm->maxs, point, ps->clientNum, pm->tracemask);
			if (trace->startsolid)
				continue;

			ps->origin = point;
			point[2] = ps->origin[2] - 1.0f - 0.25f;
			PM_PlayerTrace(pm, trace, ps->origin, pm->mins, pm->maxs, point, ps->clientNum, pm->tracemask);
			pml->groundTrace = *trace;
			ps->origin = ps->origin + (point - ps->origin) * trace->fraction;
			return true;
		}

		ps->groundEntityNum = ENTITYNUM_NONE;
		pml->groundPlane = 0;
		pml->almostGroundPlane = 0;
		pml->walking = 0;
		Sim::Jump_ClearState(ps);
		return false;
	}

	void PM_GroundTraceMissed(pmove_t* pm, pml_t* pml)
	{
		playerState_s* ps = pm->ps;
		trace_t trace = {};
		vec3 point = ps->origin;

		if (ps->groundEntityNum == ENTITYNUM_NONE)
		{
			point[2] -= 1.0f;
			PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, point, ps->clientNum, pm->tracemask);
			pml->almostGroundPlane = trace.fraction != 1.0f;
		}
		else
		{
			point[2] -= 64.0f;
			PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, point, ps->clientNum, pm->tracemask);
			// The engine also fires ANIM_ET_JUMP / ANIM_ET_JUMPBK here; nothing in the simulator animates.
			pml->almostGroundPlane = trace.fraction < 0.015625f;
		}

		ps->groundEntityNum = ENTITYNUM_NONE;
		pml->groundPlane = 0;
		pml->walking = 0;
	}

	void PM_CrashLand(playerState_s* ps, pml_t* pml)
	{
		static const dvar_s* fallDamageMinHeight = Dvar::Find("bg_fallDamageMinHeight");
		static const dvar_s* fallDamageMaxHeight = Dvar::Find("bg_fallDamageMaxHeight");

		// The engine's registered defaults, so a shim that never declared them still runs vanilla.
		const float minHeight = fallDamageMinHeight ? fallDamageMinHeight->current.value : 128.0f;
		const float maxHeight = fallDamageMaxHeight ? fallDamageMaxHeight->current.value : 300.0f;

		// Solve the step's fall for the speed at contact: the landing is graded by the height that
		// speed implies, not by how far the player actually fell.
		const float dist = pml->previous_origin[2] - ps->origin[2];
		const float vel = pml->previous_velocity[2];
		const float acc = -static_cast<float>(ps->gravity);
		const float a = acc * 0.5f;
		const float den = vel * vel - a * 4.0f * dist;
		if (den < 0.0f)
			return;

		const float t = (-vel - sqrtf(den)) / (a * 2.0f);
		const float landVel = (t * acc + vel) * -1.0f;
		const float fallHeight = landVel * landVel / (static_cast<float>(ps->gravity) * 2.0f);

		int damage = 0;
		if (minHeight < maxHeight)
		{
			if (minHeight >= fallHeight
				|| (pml->groundTrace.surfaceFlags & SURF_NODAMAGE)
				|| ps->pm_type >= PM_DEAD)
				damage = 0;
			else if (maxHeight > fallHeight)
				damage = std::clamp(static_cast<int>((fallHeight - minHeight) / (maxHeight - minHeight) * 100.0f), 0, 100);
			else
				damage = 100;
		}

		int viewDip = 0;
		if (fallHeight > 12.0f)
			viewDip = std::min(static_cast<int>((fallHeight - 12.0f) / 26.0f * 4.0f + 4.0f), 24);

		const int surfaceType = GroundSurfaceType(pml);

		if (damage)
		{
			if (damage >= 100 || (pml->groundTrace.surfaceFlags & SURF_SLICK))
			{
				ps->velocity *= 0.67000002f;
			}
			else
			{
				int stunTime = std::min(35 * damage + 500, 2000);
				float speedMult = 0.5f;
				if (stunTime > 500)
					speedMult = stunTime < 1500
						? 0.5f - (static_cast<float>(stunTime) - 500.0f) / 1000.0f * 0.300000011920929f
						: 0.2f;

				ps->pm_time = stunTime;
				ps->pm_flags |= PMF_TIME_HARDLANDING;
				ps->velocity *= speedMult;
			}
			AddEvent(ps, DamageLandingForSurface(pml), damage);
		}
		else if (fallHeight > 4.0f)
		{
			if (fallHeight >= 12.0f)
			{
				ps->velocity *= 0.67000002f;
				AddEvent(ps, HardLandingForSurface(pml), viewDip);
			}
			else if (fallHeight >= 8.0f)
				AddEvent(ps, MediumLandingForSurface(pml), surfaceType);
			else
				AddEvent(ps, LightLandingForSurface(pml), surfaceType);
		}
	}

	void PM_AddTouchEnt(pmove_t* pm, int entity_num)
	{
		if (entity_num == ENTITYNUM_WORLD || pm->numtouch == MAXTOUCH)
			return;

		for (int i = 0; i < pm->numtouch; i++)
		{
			if (pm->touchents[i] == entity_num)
				return;
		}
		pm->touchents[pm->numtouch++] = entity_num;
	}
}
