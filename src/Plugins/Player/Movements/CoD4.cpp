#include "CoD4.hpp"

#define player_spectateSpeedScale 1.0f

void BG_AddPredictableEventToPlayerstate(int event, int parms, playerState_s* ps)
{
	ps->events[ps->eventSequence & 3] = static_cast<uint8_t>(event);
	ps->eventParms[ps->eventSequence & 3] = static_cast<uint8_t>(parms);
	ps->eventSequence = static_cast<uint8_t>(ps->eventSequence + 1);
}

namespace IW3SR::Addons
{
	float CoD4::CmdScale(playerState_s* ps, usercmd_s* cmd)
	{
		const float fmove = static_cast<float>(cmd->forwardmove);
		const float rmove = static_cast<float>(cmd->rightmove);

		float max = abs(fmove);
		if (abs(rmove) > max)
			max = abs(rmove);

		if (max == 0.0f)
			return 0.0f;

		const float total = sqrt(fmove * fmove + rmove * rmove);
		float scale = static_cast<float>(ps->speed) * max / (total * 127.0f);

		if ((ps->pm_flags & PMF_WALKING) || ps->leanf != 0.0f)
			scale *= 0.4f;
		if (ps->pm_type == PM_NOCLIP)
			scale *= 3.0f;
		if (ps->pm_type == PM_UFO)
			scale *= 6.0f;
		if (ps->pm_type == PM_SPECTATOR)
			return scale * player_spectateSpeedScale;
		return scale;
	}

	void CoD4::ProjectVelocity(const vec3& velIn, const vec3& normal, vec3& out)
	{
		const float lengthSq2D = velIn[0] * velIn[0] + velIn[1] * velIn[1];

		if (std::abs(normal[2]) < 0.001f || lengthSq2D == 0.0f)
		{
			out = velIn;
			return;
		}
		const float newZ = -(normal[0] * velIn[0] + normal[1] * velIn[1]) / normal[2];
		const float originalLengthSq = lengthSq2D + velIn[2] * velIn[2];
		const float adjustedLengthSq = lengthSq2D + newZ * newZ;
		const float lengthScale = std::sqrt(originalLengthSq / adjustedLengthSq);

		if (lengthScale < 1.0f || newZ < 0.0f || velIn[2] > 0.0f)
		{
			out[0] = velIn[0] * lengthScale;
			out[1] = velIn[1] * lengthScale;
			out[2] = newZ * lengthScale;
		}
	}

	void CoD4::JumpClearState(playerState_s* ps)
	{
		ps->pm_flags &= ~PMF_JUMPING;
		ps->jumpOriginZ = 0.0;
	}

	void CoD4::CrashLand(playerState_s* ps, pml_t* pml, bool hardlanding)
	{
		const float bg_fallDamageMinHeight = Dvar::Get<float>("bg_fallDamageMinHeight");
		const float bg_fallDamageMaxHeight = Dvar::Get<float>("bg_fallDamageMaxHeight");

		float vel = pml->previous_velocity[2];
		float gravity = (float)ps->gravity;
		float acc = -gravity;
		float a = acc * 0.5f;
		float dist = pml->previous_origin[2] - ps->origin[2];
		float den = vel * vel - dist * (a * 4.0f);

		if (den < 0.0f)
			return;

		float sqrtDen = sqrtf(den);
		float t = (-vel - sqrtDen) / (a * 2.0f);
		float landVel = (t * acc + vel) * -1.0f;
		float fallHeight = (landVel * landVel) / (2.0f * gravity);

		int damage;
		if (bg_fallDamageMinHeight < bg_fallDamageMaxHeight)
		{
			if (bg_fallDamageMinHeight < fallHeight && (pml->groundTrace.surfaceFlags & 1) == 0
				&& ps->pm_type < PM_DEAD)
			{
				if (bg_fallDamageMaxHeight <= fallHeight)
				{
					damage = 100;
				}
				else
				{
					float ratio = (fallHeight - bg_fallDamageMinHeight)
						/ (bg_fallDamageMaxHeight - bg_fallDamageMinHeight) * 100.0f;
					int d = (int)ratio;
					damage = (d >= 100) ? 100 : (d > 0 ? d : 0);
				}
			}
			else
			{
				damage = 0;
			}
		}
		else
		{
			damage = 0;
		}
		int viewDip;
		if (fallHeight > 12.0f)
		{
			viewDip = (int)((fallHeight - 12.0f) / 26.0f * 4.0f + 4.0f);
			if (viewDip > 24)
				viewDip = 24;
		}
		else
		{
			viewDip = 0;
		}
		int surfaceFlags = pml->groundTrace.surfaceFlags;

		int v12;
		if (surfaceFlags & SURF_NOSTEPS)
		{
			v12 = 0;
		}
		else
		{
			v12 = (int)((surfaceFlags >> 20) & (SURF_NOIMPACT | SURF_LADDER | SURF_SKY | SURF_SLICK | SURF_NODAMAGE));
		}
		if (damage)
		{
			float scale;
			if (damage >= 100 || (surfaceFlags & SURF_SLICK) != 0)
			{
				scale = 0.6700000166893005f;
				ps->velocity[0] = ps->velocity[0] * scale;
				ps->velocity[1] = ps->velocity[1] * scale;
				ps->velocity[2] = scale * ps->velocity[2];
			}
			else
			{
				int time = 35 * damage + 500;
				float velocity_decrease_scale;

				if (time > 2000)
				{
					velocity_decrease_scale = 0.2f;
					time = 2000;
				}
				else if (time > 500)
				{
					if (time < 1500)
						velocity_decrease_scale =
							0.5f - ((float)(35 * damage + 500) - 500.0f) / 1000.0f * 0.300000011920929f;
					else
						velocity_decrease_scale = 0.2f;
				}
				else
				{
					velocity_decrease_scale = 0.5f;
				}
				ps->pm_flags |= PMF_TIME_HARDLANDING;
				ps->pm_time = time;
				ps->velocity[0] = ps->velocity[0] * velocity_decrease_scale;
				ps->velocity[1] = ps->velocity[1] * velocity_decrease_scale;
				ps->velocity[2] = velocity_decrease_scale * ps->velocity[2];
			}
			// Damage landing surface event
			if ((surfaceFlags & SURF_NOSTEPS) == 0)
			{
				int v17 = (surfaceFlags >> 20) & 0x1F;
				if (v17)
				{
					int ev = v17 + EV_LANDING_PAIN_FIRST;
					if (ev)
						BG_AddPredictableEventToPlayerstate(ev, damage, ps);
				}
			}
		}
		else if (fallHeight > 4.0f)
		{
			if (fallHeight >= 8.0f)
			{
				if (fallHeight >= 12.0f)
				{
					// Hard landing
					if (hardlanding)
					{
						ps->velocity[0] = ps->velocity[0] * 0.6700000166893005;
						ps->velocity[1] = ps->velocity[1] * 0.6700000166893005;
						ps->velocity[2] = 0.6700000166893005 * ps->velocity[2];
						if ((surfaceFlags & SURF_NOSTEPS) == 0)
						{
							int v21 = (surfaceFlags >> 20) & 0x1F;
							if (v21)
							{
								int v22 = v21 + EV_LANDING_FIRST;
								if (v22)
								{
									BG_AddPredictableEventToPlayerstate(v22, viewDip, ps);
								}
							}
						}
					}
				}
				else
				{
					// Medium landing (run footstep)
					if ((surfaceFlags & SURF_NOSTEPS) == 0)
					{
						int v20 = (surfaceFlags >> 20) & 0x1F;
						if (v20)
						{
							v20 = (v20 & ~0xFF) | (EV_FOOTSTEP_RUN & 0xFF);
							BG_AddPredictableEventToPlayerstate(v20, v12, ps);
						}
					}
				}
			}
			else
			{
				// Light landing (walk footstep)
				if ((surfaceFlags & SURF_NOSTEPS) == 0)
				{
					int v19 = (surfaceFlags >> 20) & 0x1F;
					if (v19)
					{
						v19 = (v19 & ~0xFF) | (EV_FOOTSTEP_WALK & 0xFF);
						BG_AddPredictableEventToPlayerstate(v19, v12, ps);
					}
				}
			}
		}
	}
}
