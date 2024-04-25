#include "Q3.hpp"

#define OVERCLIP 1.001f
#define MAX_CLIP_PLANES 5
#define CPM_PM_CLIPTIME 200

namespace IW3SR::Addons
{
	void Q3::WalkMove(pmove_t* pm, pml_t* pml)
	{
		const auto pm_duck_scale = 0.5f;
		const auto pm_prone_scale = 0.25f;
		const auto pm_accelerate = 15.0f;
		const auto pm_slick_accelerate = 15.0f;
		const auto pm_crouch_accelerate = 15.0f;
		const auto pm_prone_accelerate = 10.0f;

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

		Math::VectorNormalize3(pml->forward);
		Math::VectorNormalize3(pml->right);

		float speed = static_cast<float>(pm->ps->speed);

		vec3 wishvel;
		for (int i = 0; i < 3; i++)
			wishvel[i] = pml->forward[i] * forwardmove + pml->right[i] * rightmove;
		vec3 wishdir = wishvel;

		float wishspeed;
		wishdir.Normalize(wishspeed);
		wishspeed *= scale;

		// Clamp the speed lower if ducking
		if ((pm->ps->pm_flags & PMF_DUCKED) && (wishspeed > speed * pm_duck_scale))
			wishspeed = speed * pm_duck_scale;
		// Clamp the speed lower if prone
		else if ((pm->ps->pm_flags & PMF_PRONE) && (wishspeed > speed * pm_prone_scale))
			wishspeed = speed * pm_prone_scale;

		// When a player gets hit, he temporarily loses full control, which allows him to be moved a bit
		float accelerate;
		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
			accelerate = pm_slick_accelerate;
		else if (pm->ps->pm_flags & PMF_DUCKED)
			accelerate = pm_crouch_accelerate;
		else if (pm->ps->pm_flags & PMF_PRONE)
			accelerate = pm_prone_accelerate;
		else
			accelerate = pm_accelerate;

		AccelerateWalk(wishdir, pml, pm->ps, wishspeed, accelerate);

		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || (pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
			pm->ps->velocity[2] -= static_cast<float>(pm->ps->gravity) * pml->frametime;

		float vel = VectorLength3(pm->ps->velocity);

		// Slide along the ground plane
		ClipVelocity(pm->ps->velocity, pml->groundTrace.normal, pm->ps->velocity, OVERCLIP);

		// Don't decrease velocity when going up or down a slope
		Math::VectorNormalize3(pm->ps->velocity);
		VectorScale3(pm->ps->velocity, vel, pm->ps->velocity);

		// Don't do anything if standing still
		if (pm->ps->velocity[0] == 0.0f && pm->ps->velocity[1] == 0.0f)
			return;

		StepSlideMove(pm, pml, false);
	}

	void Q3::AirMove(pmove_t* pm, pml_t* pml)
	{
		const auto cpm_airstopAccelerate = 3.0f; // 2.5
		const auto cpm_strafeAccelerate = 70.0f;

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

		Math::VectorNormalize3(pml->forward);
		Math::VectorNormalize3(pml->right);

		// Determine x and y parts of velocity
		for (auto i = 0; i < 2; i++)
			wishvel[i] = pml->forward[i] * fmove + pml->right[i] * smove;

		wishvel[2] = 0; // Zero out z part of velocity
		VectorCopy3(wishvel, wishdir);
		wishspeed = Math::VectorNormalize3(wishdir);
		wishspeed *= scale;

		float accel;
		const float wishspeed2 = wishspeed;

		if (DotProduct3(ps->velocity, wishdir) < 0)
			accel = cpm_airstopAccelerate;
		else
			accel = 1.0f;

		// if (ps->movementDir == 2 || ps->movementDir == 6)
		if (pm->cmd.forwardmove == 0 && pm->cmd.rightmove != 0)
		{
			if (wishspeed > 30.0f)
				wishspeed = 30.0f;
			accel = cpm_strafeAccelerate;
		}
		Accelerate(ps, pml, wishdir, wishspeed, accel);
		AirControl(pm, pml, wishdir, wishspeed2);

		if (pml->groundPlane)
			ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity, OVERCLIP);

		// RampSliding and RampJumping
		StepSlideMove(pm, pml, true);
	}

	// https://github.com/xzero450/revolution/blob/8d0b37ba438e65e19d5a5e77f5b9c2076b7900bc/game/bg_promode.c#L303
	void Q3::AirControl(pmove_t* pm, pml_t* pml, vec3_t wishdir_b, float wishspeed_b)
	{
		const auto ps = pm->ps;

		// if (pm->cmd.forwardmove == 0.0f || wishspeed_b == 0.0)
		if ((ps->movementDir && ps->movementDir != 4) || wishspeed_b == 0.0f)
			return;

		const auto cpm_airControl = 150.0f;
		const auto cpm_airAccelerate = 1.0f;

		const float zspeed = ps->velocity[2];
		ps->velocity[2] = 0.0f;

		const float speed = Math::VectorNormalize3(ps->velocity);
		const float dot = DotProduct3(ps->velocity, wishdir_b);
		float k = 32.0f * cpm_airControl * dot * dot * pml->frametime * cpm_airAccelerate;

		if (dot > 0)
		{
			for (auto i = 0; i < 2; i++)
				ps->velocity[i] = ps->velocity[i] * speed + wishdir_b[i] * k;
			Math::VectorNormalize3(ps->velocity);
		}
		for (auto i = 0; i < 2; i++)
			ps->velocity[i] *= speed;
		ps->velocity[2] = zspeed;
	}

	// https://github.com/xzero450/revolution/blob/master/game/bg_pmove.c#L221
	void Q3::Accelerate(playerState_s* ps, pml_t* pml, vec3_t wishdir_b, float wishspeed_b, float accel_b)
	{
		const float currentspeed = DotProduct3(ps->velocity, wishdir_b);
		const float addspeed = wishspeed_b - currentspeed;

		if (addspeed <= 0)
			return;

		float accelspeed = accel_b * pml->frametime * wishspeed_b;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (auto i = 0; i < 3; i++)
			ps->velocity[i] += accelspeed * wishdir_b[i];
	}

	void Q3::AccelerateWalk(float* wishdir, pml_t* pml, playerState_s* ps, float wishspeed, float accel)
	{
		PMove::DisableSprint(ps);

		const float currentspeed = DotProduct3(ps->velocity, wishdir);
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

		for (auto i = 0; i < 3; i++)
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

		const float speed = vel.Length();
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

	void Q3::ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
	{
		float backoff = DotProduct3(in, normal);
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
		VectorCopy3(pm->ps->velocity, primal_velocity);

		if (gravity)
		{
			VectorCopy3(pm->ps->velocity, end_velocity);
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
			VectorCopy3(pml->groundTrace.normal, planes[0]);
		}
		else
			numplanes = 0;

		// Never turn against original velocity
		planes[numplanes] = pm->ps->velocity;
		planes[numplanes].Normalize();
		numplanes++;

		for (bumpcount = 0; bumpcount < NUM_BUMPS; bumpcount++)
		{
			// Calculate position we are trying to move to
			VectorMultiplyAdd3(pm->ps->origin, time_left, pm->ps->velocity, end);

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
				Math::VectorLerp3(pm->ps->origin, end, trace.fraction, end_pos);
				VectorCopy3(end_pos, pm->ps->origin);
			}
			// Moved the entire distance
			if (trace.fraction == 1.0f)
				break;

			// Save entity for contact
			PM_AddTouchEnt(pm, pm->ps->groundEntityNum);
			time_left -= time_left * trace.fraction;

			if (numplanes >= MAX_CLIP_PLANES)
			{
				VectorClear3(pm->ps->velocity);
				return true;
			}
			// If this is the same plane we hit before, nudge velocity out along it,
			// which fixes some epsilon issues with non-axial planes
			for (i = 0; i < numplanes; i++)
			{
				if (DotProduct3(trace.normal, planes[i]) > 0.99f)
				{
					VectorAdd3(trace.normal, pm->ps->velocity, pm->ps->velocity);
					break;
				}
			}
			if (i < numplanes)
				continue;

			VectorCopy3(trace.normal, planes[numplanes]);
			numplanes++;

			// Modify velocity so it parallels all of the clip planes
			// Find a plane that it enters
			for (int i = 0; i < numplanes; i++)
			{
				into = DotProduct3(pm->ps->velocity, planes[i]);

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
					if (DotProduct3(clip_velocity, planes[j]) >= 0.1f)
						continue;

					// Try clipping the move to the plane
					ClipVelocity(clip_velocity, planes[j], clip_velocity, OVERCLIP);
					ClipVelocity(end_clip_velocity, planes[j], end_clip_velocity, OVERCLIP);

					// See if it goes back into the first clip plane
					if (DotProduct3(clip_velocity, planes[i]) >= 0)
						continue;

					// Slide the original velocity along the crease
					CrossProduct3(planes[i], planes[j], dir);
					Math::VectorNormalize3(dir);

					float d = DotProduct3(dir, pm->ps->velocity);
					VectorScale3(dir, d, clip_velocity);

					CrossProduct3(planes[i], planes[j], dir);
					Math::VectorNormalize3(dir);

					d = DotProduct3(dir, end_velocity);
					VectorScale3(dir, d, end_clip_velocity);

					// See if there is a third plane the the new move enters
					for (int k = 0; k < numplanes; k++)
					{
						if (k == i || k == j)
							continue;

						// Move doesn't interact with the plane
						if (DotProduct3(clip_velocity, planes[k]) >= 0.1f)
							continue;

						// Stop dead at a tripple plane interaction
						VectorClear3(pm->ps->velocity);
						return true;
					}
				}
				// If we have fixed all interactions, try another move
				VectorCopy3(clip_velocity, pm->ps->velocity);
				VectorCopy3(end_clip_velocity, end_velocity);
				break;
			}
		}
		if (gravity)
			VectorCopy3(end_velocity, pm->ps->velocity);

		// Don't change velocity if in a timer, clipping is caused by this
		if (pm->ps->pm_time)
			VectorCopy3(primal_velocity, pm->ps->velocity);
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
		VectorCopy3(pm->ps->origin, start_o);
		VectorCopy3(pm->ps->velocity, start_v);

		// We got exactly where we wanted to go first try
		if (!SlideMove(pm, pml, gravity))
			return;

		VectorCopy3(start_o, down);
		down[2] -= jump_stepSize;

		PM_PlayerTrace(pm, &trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
		VectorSet3(up, 0, 0, 1);

		// Never step up when you still have up velocity // org
		if (pm->ps->velocity[2] > 0.0f && (trace.fraction == 1.0f || DotProduct3(trace.normal, up) < 0.7f))
			return;

		VectorCopy3(pm->ps->origin, down_o);
		VectorCopy3(pm->ps->velocity, down_v);
		VectorCopy3(start_o, up);
		up[2] += jump_stepSize;

		// Test the player position if they were a stepheight higher
		PM_PlayerTrace(pm, &trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
		Math::VectorLerp3(pm->ps->origin, up, trace.fraction, endpos); // Q3 trace

		// Can't step up
		if (trace.allsolid)
			return;
		float stepSize = endpos[2] - start_o[2];

		// Try slidemove from this position
		VectorCopy3(endpos, pm->ps->origin);
		VectorCopy3(start_v, pm->ps->velocity);

		SlideMove(pm, pml, gravity);

		// Push down the final amount
		VectorCopy3(pm->ps->origin, down);
		down[2] -= stepSize;

		PM_PlayerTrace(pm, &trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
		Math::VectorLerp3(pm->ps->origin, down, trace.fraction, endpos); // Q3 trace

		if (!trace.allsolid)
			VectorCopy3(endpos, pm->ps->origin);

		if (trace.fraction < 1.0f)
		{
			if (use_bouncing)
			{
				if (!trace.walkable && trace.normal[2] < 0.30000001f)
				{
					VectorCopy3(start_o, pm->ps->velocity);
					VectorCopy3(start_v, pm->ps->velocity);
					return;
				}
				PM_ProjectVelocity(trace.normal, pm->ps->velocity, pm->ps->velocity);
			}
			else
			{
				// Not in air
				if (!((pm->ps->velocity[2] > 0.0f) && (trace.fraction == 1.0f || DotProduct3(trace.normal, up) < 0.7f)))
					ClipVelocity(pm->ps->velocity, trace.normal, pm->ps->velocity, OVERCLIP);
				else
					VectorCopy3(start_v, pm->ps->velocity);
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
