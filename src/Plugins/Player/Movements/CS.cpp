#include "CS.hpp"

#define sv_maxspeed 250.0f
#define sv_stopspeed 80.0f
#define sv_friction 5.5f
#define sv_accelerate 10.0f
#define sv_airaccelerate 150.0f
#define sv_airspeedcap 30.0f
#define jump_height 39.0f

#define SURF_SLOPE_NORMAL 0.7f

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
		const float fmove = pm->cmd.forwardmove * scale;
		const float smove = pm->cmd.rightmove * scale;

		pm->ps->sprintState.sprintButtonUpRequired = 1;
		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;
		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		vec3 wishvel;
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * fmove + pml->right[i] * smove;
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
		PM_StepSlideMove(pm, pml, true);
	}

	void CS::AirMove(pmove_t* pm, pml_t* pml)
	{
		const auto ps = pm->ps;
		float forwardmove, rightmove, wishspeed, scale = 1.0f;
		vec3 wishvel, wishdir;

		ps->sprintState.sprintButtonUpRequired = 1;
		forwardmove = pm->cmd.forwardmove;
		rightmove = pm->cmd.rightmove;

		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;

		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		// Determine x and y parts of velocity
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * forwardmove + pml->right[i] * rightmove;

		wishvel[2] = 0;
		wishdir = wishvel;
		wishspeed = glm::length(wishdir);
		wishdir = glm::normalize(wishdir);

		if (wishspeed != 0 && (wishspeed > sv_maxspeed))
		{
			wishvel *= sv_maxspeed / wishspeed;
			wishspeed = sv_maxspeed;
		}
		AirAccelerate(wishdir, wishspeed, ps, pml);
		PM_StepSlideMove(pm, pml, true);
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
		if (!trace.walkable && (ps->pm_flags & PMF_JUMPING) && ps->jumpOriginZ > ps->origin[2])
		{
			CoD4::ProjectVelocity(ps->velocity, trace.normal, ps->velocity);
			CoD4::JumpClearState(pm->ps); // Prevent double bounce
			return;
		}
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
