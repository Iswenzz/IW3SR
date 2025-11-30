#include "Q3.hpp"

#define pm_duck_scale 0.5f
#define pm_prone_scale 0.25f
#define pm_accelerate 15.0f
#define pm_slick_accelerate 15.0f
#define pm_crouch_accelerate 15.0f
#define pm_prone_accelerate 10.0f

#define cpm_air_control 150.0f
#define cpm_air_accelerate 1.0f
#define cpm_airstop_accelerate 3.0f
#define cpm_strafe_accelerate 70.0f

#define OVERCLIP 1.001f
#define CPM_PM_CLIPTIME 200
#define MAX_CLIP_PLANES 5

namespace IW3SR::Addons
{
	void Q3::WalkMove(pmove_t* pm, pml_t* pml)
	{
		if (JumpCheck(pm, pml))
		{
			AirMove(pm, pml);
			return;
		}
		Friction(pm, pml);

		const float forwardmove = pm->cmd.forwardmove;
		const float rightmove = pm->cmd.rightmove;
		usercmd_s cmd = pm->cmd;
		const float scale = CmdScale(pm->ps, &cmd);

		// Set the movementDir so clients can rotate the legs for strafing
		SetMovementDir(pm);

		// Project moves down to flat plane
		pml->forward[2] = 0;
		pml->right[2] = 0;

		// Project the pml->forward and pml->right directions onto the ground plane
		ClipVelocity(pml->forward, pml->groundTrace.normal, pml->forward, OVERCLIP);
		ClipVelocity(pml->right, pml->groundTrace.normal, pml->right, OVERCLIP);

		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		float speed = static_cast<float>(pm->ps->speed);

		vec3 wishvel;
		for (int i = 0; i < 3; i++)
			wishvel[i] = pml->forward[i] * forwardmove + pml->right[i] * rightmove;

		vec3 wishdir = wishvel;
		float wishspeed = glm::length(wishdir) * scale;
		wishdir = glm::normalize(wishdir);

		// Clamp the speed lower if ducking
		if ((pm->ps->pm_flags & PMF_DUCKED) && (wishspeed > speed * pm_duck_scale))
			wishspeed = speed * pm_duck_scale;
		// Clamp the speed lower if prone
		else if ((pm->ps->pm_flags & PMF_PRONE) && (wishspeed > speed * pm_prone_scale))
			wishspeed = speed * pm_prone_scale;

		// When a player gets hit, he temporarily loses full control, which allows him to be moved a bit
		float accelerate = pm_accelerate;
		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
			accelerate = pm_slick_accelerate;
		else if (pm->ps->pm_flags & PMF_DUCKED)
			accelerate = pm_crouch_accelerate;
		else if (pm->ps->pm_flags & PMF_PRONE)
			accelerate = pm_prone_accelerate;

		AccelerateWalk(wishdir, pml, pm->ps, wishspeed, accelerate);

		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || (pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
			pm->ps->velocity[2] -= static_cast<float>(pm->ps->gravity) * pml->frametime;

		float vel = glm::length(pm->ps->velocity);

		// Slide along the ground plane
		ClipVelocity(pm->ps->velocity, pml->groundTrace.normal, pm->ps->velocity, OVERCLIP);

		// Don't decrease velocity when going up or down a slope
		pm->ps->velocity = glm::normalize(pm->ps->velocity) * vel;

		// Don't do anything if standing still
		if (pm->ps->velocity[0] == 0.0f && pm->ps->velocity[1] == 0.0f)
			return;

		StepSlideMove(pm, pml, false);
	}

	void Q3::AirMove(pmove_t* pm, pml_t* pml)
	{
		const auto ps = pm->ps;
		float fmove, smove, wishspeed, scale = 1.0f;
		vec3 wishvel, wishdir;

		PMove::DisableSprint(ps);
		Friction(pm, pml);

		fmove = pm->cmd.forwardmove;
		smove = pm->cmd.rightmove;

		scale = CmdScale(ps, &pm->cmd);
		SetMovementDir(pm);

		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;

		pml->forward = glm::normalize(pml->forward);
		pml->right = glm::normalize(pml->right);

		// Determine x and y parts of velocity
		for (int i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * fmove + pml->right[i] * smove;

		wishvel[2] = 0;
		wishdir = wishvel;
		wishspeed = glm::length(wishdir) * scale;
		wishdir = glm::normalize(wishdir);

		float accel;
		const float wishspeed2 = wishspeed;

		if (glm::dot(ps->velocity, wishdir) < 0)
			accel = cpm_airstop_accelerate;
		else
			accel = 1.0f;

		// if (ps->movementDir == 2 || ps->movementDir == 6)
		if (pm->cmd.forwardmove == 0 && pm->cmd.rightmove != 0)
		{
			if (wishspeed > 30.0f)
				wishspeed = 30.0f;
			accel = cpm_strafe_accelerate;
		}
		Accelerate(ps, pml, wishdir, wishspeed, accel);
		AirControl(pm, pml, wishdir, wishspeed2);

		if (pml->groundPlane)
			ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity, OVERCLIP);

		// Ramp sliding and ramp jumping
		StepSlideMove(pm, pml, true);
	}

	void Q3::GroundTrace(pmove_t* pm, pml_t* pml)
	{
		vec3 point;
		trace_t trace = {};

		point[0] = pm->ps->origin[0];
		point[1] = pm->ps->origin[1];
		point[2] = pm->ps->origin[2] - 0.25f;

		PM_PlayerTrace(pm, &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		pml->groundTrace = trace;

		// Do something corrective if the trace starts in a solid...
		if (trace.allsolid && !PM_CorrectAllSolid(pm, pml, &trace))
			return;

		// If the trace didn't hit anything, we are in free fall
		if (trace.fraction == 1.0f)
		{
			PM_GroundTraceMissed(pm, pml);
			pml->groundPlane = false;
			pml->walking = false;
			return;
		}
		// Check if getting thrown off the ground
		if (pm->ps->velocity[2] > 0.0f && glm::dot(pm->ps->velocity, trace.normal) > 10.0f)
		{
			// Go into jump animation
			if (pm->cmd.forwardmove >= 0)
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			else
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;

			pm->ps->groundEntityNum = ENTITYNUM_NONE;
			pml->groundPlane = false;
			pml->walking = false;
			return;
		}
		// Slopes that are too steep will not be considered onground
		if (trace.normal[2] < 0.7f)
		{
			pm->ps->groundEntityNum = ENTITYNUM_NONE;
			pml->groundPlane = true;
			pml->walking = false;
			return;
		}
		pml->groundPlane = true;
		pml->walking = true;

		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
			PM_CrashLand(pm->ps, pml);

		switch (trace.hitType)
		{
		case TRACE_HITTYPE_ENTITY:
			pm->ps->groundEntityNum = trace.hitId;
			break;
		case TRACE_HITTYPE_DYNENT_MODEL:
		case TRACE_HITTYPE_DYNENT_BRUSH:
			pm->ps->groundEntityNum = 1022;
			break;
		default:
			pm->ps->groundEntityNum = 1023;
		}
	}

	// https://github.com/xzero450/revolution/blob/8d0b37ba438e65e19d5a5e77f5b9c2076b7900bc/game/bg_promode.c#L303
	void Q3::AirControl(pmove_t* pm, pml_t* pml, const vec3& wishdir_b, float wishspeed_b)
	{
		const auto ps = pm->ps;

		// if (pm->cmd.forwardmove == 0.0f || wishspeed_b == 0.0)
		if ((ps->movementDir && ps->movementDir != 4) || wishspeed_b == 0.0f)
			return;

		const float zspeed = ps->velocity[2];
		ps->velocity[2] = 0.0f;

		const float speed = glm::length(ps->velocity);
		ps->velocity = glm::normalize(ps->velocity);
		const float dot = glm::dot(ps->velocity, wishdir_b);
		float k = 32.0f * cpm_air_control * dot * dot * pml->frametime * cpm_air_accelerate;

		if (dot > 0)
		{
			for (int i = 0; i < 2; i++)
				ps->velocity[i] = ps->velocity[i] * speed + wishdir_b[i] * k;
			ps->velocity = glm::normalize(ps->velocity);
		}
		for (int i = 0; i < 2; i++)
			ps->velocity[i] *= speed;
		ps->velocity[2] = zspeed;
	}

	// https://github.com/xzero450/revolution/blob/master/game/bg_pmove.c#L221
	void Q3::Accelerate(playerState_s* ps, pml_t* pml, const vec3& wishdir_b, float wishspeed_b, float accel_b)
	{
		const float currentspeed = glm::dot(ps->velocity, wishdir_b);
		const float addspeed = wishspeed_b - currentspeed;

		if (addspeed <= 0)
			return;

		float accelspeed = accel_b * pml->frametime * wishspeed_b;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (int i = 0; i < 3; i++)
			ps->velocity[i] += accelspeed * wishdir_b[i];
	}

	void Q3::AccelerateWalk(const vec3& wishdir, pml_t* pml, playerState_s* ps, float wishspeed, float accel)
	{
		PMove::DisableSprint(ps);

		const float currentspeed = glm::dot(ps->velocity, wishdir);
		const float addspeed = wishspeed - currentspeed;

		if (addspeed <= 0)
			return;

		// Fix for slow prone, crouch & ads movement
		if (ps->viewHeightCurrent == 40.0f || wishspeed < 100.0f)
			wishspeed = 100.0f;
		else if (ps->viewHeightCurrent == 11.0f)
			wishspeed = 80.0f;

		float accelspeed = accel * pml->frametime * wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (int i = 0; i < 3; i++)
			ps->velocity[i] += accelspeed * wishdir[i];
	}

	bool Q3::JumpCheck(pmove_t* pm, pml_t* pml)
	{
		if (pm->ps->pm_flags & PMF_RESPAWNED)
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

		const auto jump_height = Dvar::Get<float>("jump_height");
		float jump_velocity = sqrt(static_cast<float>(pm->ps->gravity) * (jump_height + jump_height));

		// Jump
		pml->groundPlane = false;
		pml->almostGroundPlane = false;
		pml->walking = false;
		pm->ps->pm_flags = pm->ps->pm_flags & 0xFFFFFE7F | PMF_JUMPING;
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pm->ps->velocity[2] = jump_velocity;	 // Q3 = 270, COD4 = 250
		pm->ps->jumpOriginZ = pm->ps->origin[2]; // What if we enable this for q3 too?
		pm->ps->pm_time = CPM_PM_CLIPTIME;		 // Wallclip

		return true;
	}

	void Q3::Friction(pmove_t* pm, pml_t* pml)
	{
		const float friction = Dvar::Get<float>("friction");

		vec3 vel = pm->ps->velocity;
		if (pml->walking)
			vel[2] = 0.0f; // Ignore slope movement

		const float speed = glm::length(vel);
		if (speed < 1.0f)
		{
			pm->ps->velocity[0] = 0.0f;
			pm->ps->velocity[1] = 0.0f; // Allow sinking underwater

			// No slow-sinking / raising movements
			if (pm->ps->pm_type == PM_SPECTATOR)
				pm->ps->velocity[2] = 0.0f;
			return;
		}
		float drop = 0.0f;
		// Apply ground friction
		if (pml->walking && !(pml->groundTrace.surfaceFlags & SURF_SLICK))
		{
			if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
			{
				const float control = speed < 100.0f ? 100.0f : speed;
				drop += control * friction * pml->frametime;
			}
		}
		// Apply flying friction
		if (pm->ps->pm_type == PM_SPECTATOR)
			drop += (speed * 5.0f * pml->frametime);

		// scale the velocity
		float newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		for (int i = 0; i < 3; i++)
			pm->ps->velocity[i] *= newspeed;
	}

	void Q3::ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce)
	{
		float backoff = glm::dot(in, normal);
		if (backoff < 0.0f)
			backoff *= overbounce;
		else
			backoff /= overbounce;

		for (int i = 0; i < 3; i++)
			out[i] = in[i] - (normal[i] * backoff);
	}

	bool Q3::SlideMove(pmove_t* pm, pml_t* pml, bool gravity)
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
			// If this is the same plane we hit before, nudge velocity out along it,
			// which fixes some epsilon issues with non-axial planes
			for (i = 0; i < numplanes; i++)
			{
				if (glm::dot(trace.normal, planes[i]) > 0.99f)
				{
					pm->ps->velocity += trace.normal;
					break;
				}
			}
			if (i < numplanes)
				continue;

			planes[numplanes] = trace.normal;
			numplanes++;

			// Modify velocity so it parallels all of the clip planes
			// Find a plane that it enters
			for (int i = 0; i < numplanes; i++)
			{
				into = glm::dot(pm->ps->velocity, planes[i]);

				// Move doesn't interact with the plane
				if (into >= 0.1f)
					continue;

				// See how hard we are hitting things
				if (-into > pml->impactSpeed)
					pml->impactSpeed = -into;

				// Slide along the plane
				ClipVelocity(pm->ps->velocity, planes[i], clip_velocity, OVERCLIP);
				ClipVelocity(end_velocity, planes[i], end_clip_velocity, OVERCLIP);

				// see if there is a second plane that the new move enters
				for (int j = 0; j < numplanes; j++)
				{
					if (j == i)
						continue;

					// Move doesn't interact with the plane
					if (glm::dot(clip_velocity, planes[j]) >= 0.1f)
						continue;

					// Try clipping the move to the plane
					ClipVelocity(clip_velocity, planes[j], clip_velocity, OVERCLIP);
					ClipVelocity(end_clip_velocity, planes[j], end_clip_velocity, OVERCLIP);

					// See if it goes back into the first clip plane
					if (glm::dot(clip_velocity, planes[i]) >= 0)
						continue;

					// Slide the original velocity along the crease
					dir = glm::cross(planes[i], planes[j]);
					dir = glm::normalize(dir);

					float d = glm::dot(dir, pm->ps->velocity);
					clip_velocity = dir * d;

					dir = glm::cross(planes[i], planes[j]);
					dir = glm::normalize(dir);

					d = glm::dot(dir, end_velocity);
					end_clip_velocity = dir * d;

					// See if there is a third plane the the new move enters
					for (int k = 0; k < numplanes; k++)
					{
						if (k == i || k == j)
							continue;

						// Move doesn't interact with the plane
						if (glm::dot(clip_velocity, planes[k]) >= 0.1f)
							continue;

						// Stop dead at a tripple plane interaction
						pm->ps->velocity = { 0, 0, 0 };
						return true;
					}
				}
				// If we have fixed all interactions, try another move
				pm->ps->velocity = clip_velocity;
				end_velocity = end_clip_velocity;
				break;
			}
		}
		if (gravity)
			pm->ps->velocity = end_velocity;

		// Don't change velocity if in a timer, clipping is caused by this
		if (pm->ps->pm_time)
			pm->ps->velocity = primal_velocity;

		return bumpcount != 0;
	}

	void Q3::StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity)
	{
		const float jump_stepSize = Dvar::Get<float>("jump_stepSize");
		const bool use_bouncing = true;
		trace_t trace = {};

		vec3 start_o, start_v, endpos;
		vec3 down_o, down_v;
		vec3 up, down;

		if (pm->ps->pm_flags & 8)
		{
			pm->ps->jumpOriginZ = 0.0;
			trace.allsolid = false;
			pm->ps->pm_flags = pm->ps->pm_flags & 0xFFFFBFFF;
		}
		else if (pml->groundPlane)
			trace.allsolid = true;
		else
		{
			trace.allsolid = false;
			if (pm->ps->pm_flags & PMF_JUMPING && pm->ps->pm_time)
			{
				pm->ps->jumpOriginZ = 0.0f;
				pm->ps->pm_flags = pm->ps->pm_flags & 0xFFFFBFFF;
			}
		}
		start_o = pm->ps->origin;
		start_v = pm->ps->velocity;

		// We got exactly where we wanted to go first try
		if (!SlideMove(pm, pml, gravity))
			return;

		down = start_o;
		down[2] -= jump_stepSize;

		PM_PlayerTrace(pm, &trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
		up = { 0, 0, 1 };

		// Never step up when you still have up velocity
		if (pm->ps->velocity[2] > 0.0f && (trace.fraction == 1.0f || glm::dot(trace.normal, up) < 0.7f))
			return;

		down_o = pm->ps->origin;
		down_v = pm->ps->velocity;
		up = start_o;
		up[2] += jump_stepSize;

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
			if (use_bouncing)
			{
				if (!trace.walkable && trace.normal[2] < 0.30000001f)
				{
					pm->ps->velocity = start_o;
					pm->ps->velocity = start_v;
					return;
				}
				PM_ProjectVelocity(trace.normal, pm->ps->velocity, pm->ps->velocity);
			}
			else
			{
				// Not in air
				if (!((pm->ps->velocity[2] > 0.0f) && (trace.fraction == 1.0f || glm::dot(trace.normal, up) < 0.7f)))
					ClipVelocity(pm->ps->velocity, trace.normal, pm->ps->velocity, OVERCLIP);
				else
					pm->ps->velocity = start_v;
			}
		}
	}

	// https://github.com/jangroothuijse/openpromode/blob/master/code/game/bg_pmove.c#L287
	float Q3::CmdScale(playerState_s* ps, usercmd_s* cmd)
	{
		const float fmove = static_cast<float>(cmd->forwardmove);
		const float rmove = static_cast<float>(cmd->rightmove);

		float max = abs(fmove);

		if (abs(rmove) > max)
			max = abs(rmove);

		if (max == 0.0f)
			return 0.0f;

		const float total = sqrt(fmove * fmove + rmove * rmove);
		return static_cast<float>(ps->speed) * max / (127.0f * total);
	}

	// https://github.com/ZdrytchX/GPP-1-1/blob/master/src/game/bg_pmove.c#L499
	void Q3::SetMovementDir(pmove_t* pm)
	{
		// Don't set dir when proned or crouched
		if (pm->ps->viewHeightCurrent == 11.0f || pm->ps->viewHeightCurrent == 40.0f)
			return;
		if (pm->cmd.forwardmove || pm->cmd.rightmove)
		{
			if (pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0)
				pm->ps->movementDir = 0; // Forward
			else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0)
				pm->ps->movementDir = 1; // Strafe Forward Left
			else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0)
				pm->ps->movementDir = 2; // Left
			else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0)
				pm->ps->movementDir = 3; // Strafe Backwards Left
			else if (pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0)
				pm->ps->movementDir = 4; // Backward
			else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0)
				pm->ps->movementDir = 5; // Strafe Backwards Right
			else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0)
				pm->ps->movementDir = 6; // Right
			else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0)
				pm->ps->movementDir = 7; // Strafe Forward Right
			return;
		}
		// If they aren't actively going directly sideways
		// change the animation to the diagonal so they don't stop too crooked
		if (pm->ps->movementDir == 2)
			pm->ps->movementDir = 1;
		else if (pm->ps->movementDir == 6)
			pm->ps->movementDir = 7;
	}
}
