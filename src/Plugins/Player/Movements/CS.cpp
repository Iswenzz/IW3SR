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

		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		// Determine x and y parts of velocity
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * fmove + pml->right[i] * smove;

		wishvel[2] = 0;
		wishdir = wishvel;
		wishspeed = glm::length(wishdir);
		wishdir = glm::normalize(wishdir);

		// wishspeed > mv->m_flMaxSpeed
		if (wishspeed != 0 && (wishspeed > sv_maxspeed))
		{
			wishvel *= sv_maxspeed / wishspeed;
			wishspeed = sv_maxspeed;
		}
		AirAccelerate(wishdir, wishspeed, ps, pml);
		PM_StepSlideMove(pm, pml, true);
		TryPlayerMove(pm, pml);
	}

	void CS::AirAccelerate(const vec3& wishdir, float wishspeed, playerState_s* ps, pml_t* pml)
	{
		float wishspeed2 = wishspeed;

		if (wishspeed2 > pm_airspeedcap)
			wishspeed2 = pm_airspeedcap;

		const float currentspeed = glm::dot(ps->velocity, wishdir);
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

		if (!glm::length(ps->velocity))
			return;

		// Assume we can move all the way from the current origin to the end point.
		end = ps->origin + ps->velocity * pml->frametime;
		PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

		// If we covered the entire distance, we are done and can return.
		if (trace.fraction == 1.0f)
			return;

		// If the plane we hit has a high z component in the normal, then it's probably a floor
		if (trace.normal[2] > SURF_SLOPE_NORMAL)
			return;

		ClipVelocity(ps->velocity, trace.normal, ps->velocity, 1.0f);
	}

	void CS::ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce)
	{
		// Determine how far along plane to slide based on incoming direction.
		const float backoff = glm::dot(in, normal) * overbounce;

		for (int i = 0; i < 3; i++)
			out[i] = in[i] - (normal[i] * backoff);

		// Iterate once to make sure we aren't still moving through the plane
		const float adjust = glm::dot(out, normal);
		if (adjust < 0.0f)
			out -= normal * adjust;
	}
}
