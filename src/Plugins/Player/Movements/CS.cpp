#include "CS.hpp"

#define sv_maxspeed 250.0f
#define sv_stopspeed 80.0f
#define sv_friction 5.5f
#define sv_accelerate 10.0f
#define sv_airaccelerate 150.0f
#define sv_airspeedcap 30.0f
#define sv_stepsize 18.0f
#define jump_height 39.0f

#define SURF_SLOPE_NORMAL 0.7f
#define OVERCLIP 1.001f
#define MAX_CLIP_PLANES 5

namespace IW3SR::Addons
{
	void CS::WalkMove(pmove_t* pm, pml_t* pml)
	{
		if (JumpCheck(pm, pml))
		{
			AirMove(pm, pml);
			return;
		}
		Friction(pm, pml);

		const float scale = CoD4::CmdScale(pm->ps, &pm->cmd);
		const float forwardmove = pm->cmd.forwardmove * scale;
		const float rightmove = pm->cmd.rightmove * scale;

		pm->ps->sprintState.sprintButtonUpRequired = 1;
		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;
		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		vec3 wishvel;
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * forwardmove + pml->right[i] * rightmove;
		wishvel[2] = 0.0f;

		vec3 wishdir = wishvel;
		float wishspeed = glm::length(wishdir);
		if (wishspeed > 0.0f)
			wishdir = glm::normalize(wishdir);

		if (wishspeed > sv_maxspeed)
		{
			wishvel *= sv_maxspeed / wishspeed;
			wishspeed = sv_maxspeed;
		}
		Accelerate(wishdir, wishspeed, pm->ps, pml);
		StepSlideMove(pm, pml, true);
	}

	void CS::AirMove(pmove_t* pm, pml_t* pml)
	{
		const float scale = CoD4::CmdScale(pm->ps, &pm->cmd);
		const float forwardmove = pm->cmd.forwardmove * scale;
		const float rightmove = pm->cmd.rightmove * scale;

		pm->ps->sprintState.sprintButtonUpRequired = 1;
		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;
		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		// Determine x and y parts of velocity
		vec3 wishvel;
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * forwardmove + pml->right[i] * rightmove;

		wishvel[2] = 0;
		vec3 wishdir = wishvel;
		float wishspeed = glm::length(wishdir);
		wishdir = glm::normalize(wishdir);

		if (wishspeed != 0 && (wishspeed > sv_maxspeed))
		{
			wishvel *= sv_maxspeed / wishspeed;
			wishspeed = sv_maxspeed;
		}
		AirAccelerate(wishdir, wishspeed, pm->ps, pml);
		StepSlideMove(pm, pml, true);
		TryPlayerMove(pm, pml);
	}

	bool CS::JumpCheck(pmove_t* pm, pml_t* pml)
	{
		if (pm->ps->pm_flags & PMF_NO_JUMP)
			return false;
		if (pm->ps->pm_flags & PMF_JUMP_HELD)
			return false;
		if (pm->ps->pm_flags & PMF_MANTLE)
			return false;
		if (pm->ps->pm_type >= PM_DEAD)
			return false;
		if (pm->ps->viewHeightTarget == 11 || pm->ps->viewHeightTarget == 40)
			return false;
		if (!(pm->cmd.buttons & PMF_JUMP_HELD))
			return false;

		float jump_velocity = sqrt(2.0f * static_cast<float>(pm->ps->gravity) * jump_height);

		pml->groundPlane = false;
		pml->almostGroundPlane = false;
		pml->walking = false;
		pm->ps->pm_flags &= ~(PMF_TIME_HARDLANDING | PMF_TIME_KNOCKBACK);
		pm->ps->pm_flags |= PMF_JUMPING;
		pm->ps->pm_time = 0;
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pm->ps->velocity[2] = pm->ps->velocity[2] > 0.0f ? pm->ps->velocity[2] + jump_velocity : jump_velocity;
		pm->ps->jumpOriginZ = pm->ps->origin[2];
		return true;
	}

	void CS::Friction(pmove_t* pm, pml_t* pml)
	{
		float speed = glm::length(pm->ps->velocity);

		if (speed < 1.0f)
		{
			pm->ps->velocity[0] = 0.0f;
			pm->ps->velocity[1] = 0.0f;
			return;
		}
		float drop = 0.0f;

		if (pml->walking)
		{
			float control = speed < sv_stopspeed ? sv_stopspeed : speed;
			drop += control * sv_friction * pml->frametime;
		}
		float newspeed = speed - drop;
		if (newspeed < 0.0f)
			newspeed = 0.0f;

		newspeed /= speed;
		pm->ps->velocity *= newspeed;
	}

	void CS::Accelerate(const vec3& wishdir, float wishspeed, playerState_s* ps, pml_t* pml)
	{
		const float currentspeed = glm::dot(ps->velocity, wishdir);
		const float addspeed = wishspeed - currentspeed;

		if (addspeed <= 0.0f)
			return;

		float accelspeed = sv_accelerate * pml->frametime * wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		ps->velocity += wishdir * accelspeed;
	}

	void CS::AirAccelerate(const vec3& wishdir, float wishspeed, playerState_s* ps, pml_t* pml)
	{
		float wishspeed2 = wishspeed;

		if (wishspeed2 > sv_airspeedcap)
			wishspeed2 = sv_airspeedcap;

		const float currentspeed = glm::dot(ps->velocity, wishdir);
		const float addspeed = wishspeed2 - currentspeed;

		if (addspeed > 0)
		{
			float accelspeed = pml->frametime * sv_airaccelerate * wishspeed;
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

		end = ps->origin + ps->velocity * pml->frametime;
		PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

		if (trace.fraction == 1.0f)
			return;

		if (trace.normal[2] > SURF_SLOPE_NORMAL)
			return;

		if (!trace.walkable && trace.normal[2] < 0.30000001f)
			return;

		// CoD4 bounce
		if (!trace.walkable && trace.normal[2] >= 0.30000001f && (pm->ps->pm_flags & PMF_JUMPING)
			&& pm->ps->jumpOriginZ > pm->ps->origin[2])
			return;

		ClipVelocity(ps->velocity, trace.normal, ps->velocity, OVERCLIP);
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

	float CS::PermuteRestrictiveClipPlanes(const vec3& velocity, int planeCount, vec3* planes, int* permutation)
	{
		float parallel[MAX_CLIP_PLANES];

		for (int planeIndex = 0; planeIndex < planeCount; planeIndex++)
		{
			parallel[planeIndex] = glm::dot(velocity, planes[planeIndex]);
			int permutedIndex = planeIndex;
			while (permutedIndex > 0 && parallel[planeIndex] <= parallel[permutation[permutedIndex - 1]])
			{
				permutation[permutedIndex] = permutation[permutedIndex - 1];
				permutedIndex--;
			}
			permutation[permutedIndex] = planeIndex;
		}
		return parallel[permutation[0]];
	}

	bool CS::SlideMove(pmove_t* pm, pml_t* pml, bool gravity)
	{
		const int NUM_BUMPS = 4;

		vec3 planes[MAX_CLIP_PLANES];
		vec3 primal_velocity, clip_velocity, end_velocity, end_clip_velocity;
		vec3 dir, end, end_pos;

		int numplanes, bumpcount, i;
		float time_left, into;

		trace_t trace = {};
		primal_velocity = pm->ps->velocity;

		if (gravity)
		{
			end_velocity = pm->ps->velocity;
			end_velocity[2] -= static_cast<float>(pm->ps->gravity) * pml->frametime;

			pm->ps->velocity[2] = (pm->ps->velocity[2] + end_velocity[2]) * 0.5f;
			primal_velocity[2] = end_velocity[2];

			// Slide along the ground plane
			if (pml->groundPlane)
				ClipVelocity(pm->ps->velocity, pml->groundTrace.normal, pm->ps->velocity, OVERCLIP);
		}
		time_left = pml->frametime;

		// Never turn against the ground plane
		if (pml->groundPlane)
		{
			numplanes = 1;
			planes[0] = pml->groundTrace.normal;
		}
		else
			numplanes = 0;

		// Never turn against original velocity
		planes[numplanes] = pm->ps->velocity;
		planes[numplanes] = glm::normalize(planes[numplanes]);
		numplanes++;

		for (bumpcount = 0; bumpcount < NUM_BUMPS; bumpcount++)
		{
			// Calculate position we are trying to move to
			end = pm->ps->origin + pm->ps->velocity * time_left;

			// See if we can make it there
			PM_PlayerTrace(pm, &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

			// Entity is completely trapped in another solid
			if (trace.allsolid)
			{
				// Don't build up falling damage, but allow sideways acceleration
				pm->ps->velocity[2] = 0.0f;
				return true;
			}
			// Actually covered some distance
			if (trace.fraction > 0.0f)
			{
				end_pos = glm::mix(pm->ps->origin, end, trace.fraction);
				pm->ps->origin = end_pos;
			}
			// Moved the entire distance
			if (trace.fraction == 1.0f)
				break;

			// Save entity for contact
			PM_AddTouchEnt(pm, pm->ps->groundEntityNum);
			time_left -= time_left * trace.fraction;

			if (numplanes >= MAX_CLIP_PLANES)
			{
				pm->ps->velocity = { 0, 0, 0 };
				return true;
			}
			// If this is the same plane we hit before, clip and nudge velocity out along it,
			// which fixes getting stuck on non-axial planes like wedges
			for (i = 0; i < numplanes; i++)
			{
				if (glm::dot(trace.normal, planes[i]) > 0.999f)
				{
					ClipVelocity(pm->ps->velocity, trace.normal, pm->ps->velocity, OVERCLIP);
					pm->ps->velocity += trace.normal;
					break;
				}
			}
			if (i < numplanes)
				continue;

			planes[numplanes] = trace.normal;
			numplanes++;

			int permutation[MAX_CLIP_PLANES] = {};
			float into = PermuteRestrictiveClipPlanes(pm->ps->velocity, numplanes, planes, permutation);

			if (into < 0.1f)
			{
				if (-into > pml->impactSpeed)
					pml->impactSpeed = -into;

				ClipVelocity(pm->ps->velocity, planes[permutation[0]], clip_velocity, OVERCLIP);
				ClipVelocity(end_velocity, planes[permutation[0]], end_clip_velocity, OVERCLIP);

				for (int j = 1; j < numplanes; j++)
				{
					if (glm::dot(clip_velocity, planes[permutation[j]]) < 0.1f)
					{
						ClipVelocity(clip_velocity, planes[permutation[j]], clip_velocity, OVERCLIP);
						ClipVelocity(end_clip_velocity, planes[permutation[j]], end_clip_velocity, OVERCLIP);

						if (glm::dot(clip_velocity, planes[permutation[0]]) < 0.0f)
						{
							dir = glm::cross(planes[permutation[0]], planes[permutation[j]]);
							dir = glm::normalize(dir);
							float d = glm::dot(dir, pm->ps->velocity);
							clip_velocity = dir * d;
							d = glm::dot(dir, end_velocity);
							end_clip_velocity = dir * d;

							for (int k = 1; k < numplanes; k++)
							{
								if (k != j && glm::dot(clip_velocity, planes[permutation[k]]) < 0.1f)
								{
									pm->ps->velocity = { 0, 0, 0 };
									return true;
								}
							}
						}
					}
				}
				pm->ps->velocity = clip_velocity;
				end_velocity = end_clip_velocity;
			}
		}
		if (gravity)
			pm->ps->velocity = end_velocity;

		return bumpcount != 0;
	}

	void CS::StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity)
	{
		trace_t trace = {};
		vec3 start_o, start_v, endpos;
		vec3 down_o, down_v;
		vec3 up, down;

		if (pm->ps->pm_flags & PMF_LADDER)
		{
			trace.allsolid = false;
			CoD4::JumpClearState(pm->ps);
		}
		else if (pml->groundPlane)
			trace.allsolid = true;
		else
		{
			trace.allsolid = false;
			if (pm->ps->pm_flags & PMF_JUMPING && pm->ps->pm_time)
				CoD4::JumpClearState(pm->ps);
		}
		start_o = pm->ps->origin;
		start_v = pm->ps->velocity;

		// We got exactly where we wanted to go first try
		if (!SlideMove(pm, pml, gravity))
			return;

		down = start_o;
		down[2] -= sv_stepsize;

		PM_PlayerTrace(pm, &trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
		up = { 0, 0, 1 };

		// Never step up when you still have up velocity
		if (pm->ps->velocity[2] > 0.0f && (trace.fraction == 1.0f || glm::dot(trace.normal, up) < 0.7f))
			return;

		down_o = pm->ps->origin;
		down_v = pm->ps->velocity;
		up = start_o;
		up[2] += sv_stepsize;

		// Test the player position if they were a stepheight higher
		PM_PlayerTrace(pm, &trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
		endpos = glm::mix(pm->ps->origin, up, trace.fraction);

		// Can't step up
		if (trace.allsolid)
			return;
		float stepSize = endpos[2] - start_o[2];

		// Try slidemove from this position
		pm->ps->origin = endpos;
		pm->ps->velocity = start_v;

		SlideMove(pm, pml, gravity);

		// Push down the final amount
		down = pm->ps->origin;
		down[2] -= stepSize;

		PM_PlayerTrace(pm, &trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
		endpos = glm::mix(pm->ps->origin, down, trace.fraction);

		if (!trace.allsolid)
			pm->ps->origin = endpos;

		if (trace.fraction < 1.0f)
		{
			// CoD4 bounce
			if (!trace.walkable && trace.normal[2] >= 0.30000001f && (pm->ps->pm_flags & PMF_JUMPING)
				&& pm->ps->jumpOriginZ > pm->ps->origin[2])
			{
				pm->ps->velocity[2] *= 0.9f; // Tweak bounce velocity
				CoD4::ProjectVelocity(pm->ps->velocity, trace.normal, pm->ps->velocity);
				CoD4::JumpClearState(pm->ps); // Prevent double bounce
				return;
			}
			ClipVelocity(pm->ps->velocity, trace.normal, pm->ps->velocity, OVERCLIP);
		}
	}
}
