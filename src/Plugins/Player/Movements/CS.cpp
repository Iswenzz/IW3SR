#include "CS.hpp"

#define sv_maxspeed 320.0f
#define pm_accel 100.0f
#define pm_airspeedcap 30.0f

#define SURF_SLOPE_NORMAL 0.7f

namespace IW3SR::Addons
{
	void CS::AirMove(pmove_t* pm, pml_t* pml)
	{
		const auto ps = pm->ps;
		float fmove, smove, wishspeed, scale = 1.0f;
		vec3 wishvel, wishdir;

		PMove::DisableSprint(ps);

		fmove = pm->cmd.forwardmove;
		smove = pm->cmd.rightmove;

		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;

		Math::VectorNormalize3(pml->forward);
		Math::VectorNormalize3(pml->right);

		// Determine x and y parts of velocity
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * fmove + pml->right[i] * smove;

		wishvel[2] = 0; // Zero out z part of velocity
		VectorCopy3(wishvel, wishdir);
		wishspeed = Math::VectorNormalize3(wishdir);

		// wishspeed > mv->m_flMaxSpeed
		if (wishspeed != 0 && (wishspeed > sv_maxspeed))
		{
			VectorScale3(wishvel, sv_maxspeed / wishspeed, wishvel);
			wishspeed = sv_maxspeed;
		}
		AirAccelerate(wishdir, wishspeed, ps, pml);
		PM_StepSlideMove(pm, pml, true);
		TryPlayerMove(pm, pml);
	}

	void CS::AirAccelerate(vec3_t wishdir, float wishspeed, playerState_s* ps, pml_t* pml)
	{
		float wishspeed2 = wishspeed;

		if (wishspeed2 > pm_airspeedcap)
			wishspeed2 = pm_airspeedcap;

		const float currentspeed = DotProduct3(ps->velocity, wishdir);
		const float addspeed = wishspeed2 - currentspeed;

		if (addspeed > 0)
		{
			float accelspeed = pml->frametime * pm_accel * wishspeed * 1.0f; // * surfacefriction
			if (accelspeed > addspeed)
				accelspeed = addspeed;

			for (int i = 0; i < 3; i++)
				ps->velocity[i] += wishdir[i] * accelspeed;
		}
	}

	void CS::TryPlayerMove(pmove_t* pm, pml_t* pml)
	{
		vec3 end;
		trace_t trace = {};
		const auto ps = pm->ps;

		if (!VectorLength3(ps->velocity))
			return;

		// Assume we can move all the way from the current origin to the end point.
		VectorMultiplyAdd3(ps->origin, pml->frametime, ps->velocity, end);
		PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

		// If we covered the entire distance, we are done and can return.
		if (trace.fraction == 1.0f)
			return;

		// If the plane we hit has a high z component in the normal, then it's probably a floor
		if (trace.normal[2] > SURF_SLOPE_NORMAL)
			return;

		ClipVelocity(ps->velocity, trace.normal, ps->velocity, 1.0f);
	}

	void CS::ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
	{
		// Determine how far along plane to slide based on incoming direction.
		const float backoff = DotProduct3(in, normal) * overbounce;

		for (int i = 0; i < 3; i++)
			out[i] = in[i] - (normal[i] * backoff);

		// Iterate once to make sure we aren't still moving through the plane
		const float adjust = DotProduct3(out, normal);
		if (adjust < 0.0f)
		{
			vec3 reduce;
			VectorScale3(normal, adjust, reduce);
			VectorSubtract3(out, reduce, out);
		}
	}
}
