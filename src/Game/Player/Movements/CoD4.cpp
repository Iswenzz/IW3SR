#include "CoD4.hpp"

#define player_spectateSpeedScale 1.0f

void BG_AddPredictableEventToPlayerstate(int event, int parms, playerState_s* ps)
{
	ps->events[ps->eventSequence & 3] = static_cast<uint8_t>(event);
	ps->eventParms[ps->eventSequence & 3] = static_cast<uint8_t>(parms);
	ps->eventSequence = static_cast<uint8_t>(ps->eventSequence + 1);
}

namespace IW3SR
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
}
