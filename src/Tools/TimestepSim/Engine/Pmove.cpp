#include "Engine/Pmove.hpp"

#include "Game/Base.hpp"
#include "Game/Player/Movements/CS.hpp"
#include "Game/Player/Movements/Q3.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace Sim
{
	namespace
	{
		Mode g_mode = Mode::COD4;

		constexpr float EQUAL_EPSILON = 0.001f;
		constexpr float PM_OVERCLIP = 1.0f + EQUAL_EPSILON;
		constexpr float DEG_TO_RAD = 0.01745329238474369f;
		constexpr float ONE_OVER_360 = 0.002777777845039964f;
		constexpr float SHORT_TO_DEGREE = 0.0054931640625f;

		constexpr int ENTITYNUM_WORLD = 1022;

		// bg_local.h:752 has PMF_PRONE as bit 0. Structs.hpp, and so Types.hpp, carries it as
		// BIT(14) | BIT(0), which aliases PMF_JUMPING: PM_CheckDuck would then clear the jump flag
		// every step and kill the whole landing slowdown. The engine value is bound here so the
		// engine half behaves as the engine does, and the mod sources keep reading theirs.
		constexpr int PMF_PRONE_BIT = BIT(0);

		// The button bits the engine actually uses. Types.hpp's BUTTON_CROUCH/BUTTON_PRONE/
		// BUTTON_ADS carry different values than the shipping usercmd layout, so the stance and
		// ads paths below name the engine's bits instead.
		constexpr int CMD_BUTTON_PRONE = 0x100;
		constexpr int CMD_BUTTON_CROUCH = 0x200;
		constexpr int CMD_BUTTON_STANCE = 0x300;
		constexpr int CMD_BUTTON_ADS = 0x800;
		constexpr int CMD_BUTTON_STANCE_HOLD = 0x1000;
		constexpr int CMD_BUTTON_NO_INPUT = 0x100000;

		// eFlags bits the movement code branches on.
		constexpr int EF_CROUCH = 0x4;
		constexpr int EF_PRONE = 0x8;
		constexpr int EF_FIRING = 0x40;
		constexpr int EF_TURRET_ACTIVE = 0x300;
		constexpr int EF_NO_INPUT = 0x200000;

		constexpr int CONTENTS_BODY = 0x2000000;

		constexpr int JUMP_LAND_SLOWDOWN_TIME = 1800;

		struct viewLerpWaypoint_s
		{
			int iFrac;
			float fViewHeight;
			int iOffset;
		};

		const viewLerpWaypoint_s viewLerp_StandCrouch[9] = {
			{ 0, 60.0f, 0 }, { 1, 59.5f, 0 }, { 4, 58.5f, 0 }, { 30, 56.0f, 0 }, { 80, 44.0f, 0 },
			{ 90, 41.5f, 0 }, { 95, 40.5f, 0 }, { 100, 40.0f, 0 }, { -1, 0.0f, 0 }
		};
		const viewLerpWaypoint_s viewLerp_CrouchStand[9] = {
			{ 0, 40.0f, 0 }, { 5, 40.5f, 0 }, { 10, 41.5f, 0 }, { 20, 44.0f, 0 }, { 70, 56.0f, 0 },
			{ 96, 58.5f, 0 }, { 99, 59.5f, 0 }, { 100, 60.0f, 0 }, { -1, 0.0f, 0 }
		};
		const viewLerpWaypoint_s viewLerp_CrouchProne[11] = {
			{ 0, 40.0f, 0 }, { 11, 38.0f, 0 }, { 22, 33.0f, 0 }, { 34, 25.0f, 0 }, { 45, 16.0f, 0 },
			{ 50, 15.0f, 0 }, { 55, 16.0f, 0 }, { 70, 18.0f, 0 }, { 90, 17.0f, 0 }, { 100, 11.0f, 0 },
			{ -1, 0.0f, 0 }
		};
		const viewLerpWaypoint_s viewLerp_ProneCrouch[8] = {
			{ 0, 11.0f, 0 }, { 5, 10.0f, 0 }, { 30, 21.0f, 0 }, { 50, 25.0f, 0 }, { 67, 31.0f, 0 },
			{ 83, 34.0f, 0 }, { 100, 40.0f, 0 }, { -1, 0.0f, 0 }
		};

		// A run that never registers a dvar still has to move like the retail defaults, so every
		// lookup carries the value bg_misc.cpp / bg_jump.cpp registers.
		float DvarFloat(const char* name, float fallback)
		{
			const dvar_s* dvar = Dvar::Find(name);
			return dvar ? dvar->current.value : fallback;
		}

		int DvarInt(const char* name, int fallback)
		{
			const dvar_s* dvar = Dvar::Find(name);
			return dvar ? dvar->current.integer : fallback;
		}

		bool DvarBool(const char* name, bool fallback)
		{
			const dvar_s* dvar = Dvar::Find(name);
			return dvar ? dvar->current.enabled : fallback;
		}

		// Vec3Normalize: divides by 1 rather than by zero on a null vector, which several call
		// sites below rely on instead of testing the length themselves.
		float Vec3Normalize(vec3& v)
		{
			const float length = std::sqrt(glm::dot(v, v));
			v *= 1.0f / (length > 0.0f ? length : 1.0f);
			return length;
		}

		float Vec3NormalizeTo(const vec3& v, vec3& out)
		{
			const float length = std::sqrt(glm::dot(v, v));
			out = v * (1.0f / (length > 0.0f ? length : 1.0f));
			return length;
		}

		// Vec2Normalize, which PM_WalkMove uses on pml->forward/right after zeroing z.
		float Vec2Normalize(vec3& v)
		{
			const float length = std::sqrt(v[0] * v[0] + v[1] * v[1]);
			const float ilength = 1.0f / (length > 0.0f ? length : 1.0f);
			v[0] *= ilength;
			v[1] *= ilength;
			return length;
		}

		float Vec2Length(const vec3& v)
		{
			return std::sqrt(v[0] * v[0] + v[1] * v[1]);
		}

		vec3 Vec3Lerp(const vec3& from, const vec3& to, float frac)
		{
			return from + (to - from) * frac;
		}

		// Sys_SnapVector. The engine's fistp rounds to nearest, ties to even, so nearbyint under
		// the default rounding mode is the faithful stand-in, not a truncating cast.
		float SnapFloat(float x)
		{
			return std::nearbyintf(x);
		}

		void Sys_SnapVector(vec3& v)
		{
			v[0] = SnapFloat(v[0]);
			v[1] = SnapFloat(v[1]);
			v[2] = SnapFloat(v[2]);
		}

		float AngleDelta(float a, float b)
		{
			const float scaled = (a - b) * ONE_OVER_360;
			return (scaled - std::floor(scaled + 0.5f)) * 360.0f;
		}

		void AngleVectors(const vec3& angles, vec3* forward, vec3* right, vec3* up)
		{
			const float yaw = angles[1] * DEG_TO_RAD;
			const float cy = std::cos(yaw);
			const float sy = std::sin(yaw);
			const float pitch = angles[0] * DEG_TO_RAD;
			const float cp = std::cos(pitch);
			const float sp = std::sin(pitch);

			if (forward)
				*forward = { cp * cy, cp * sy, -sp };

			if (right || up)
			{
				const float roll = angles[2] * DEG_TO_RAD;
				const float cr = std::cos(roll);
				const float sr = std::sin(roll);

				if (right)
					*right = { -sr * sp * cy + cr * sy, -sr * sp * sy - cr * cy, -sr * cp };
				if (up)
					*up = { cr * sp * cy + sr * sy, cr * sp * sy - sr * cy, cr * cp };
			}
		}

		float vectoyaw(const vec3& vec)
		{
			if (vec[1] == 0.0f && vec[0] == 0.0f)
				return 0.0f;

			const float yaw = std::atan2(vec[1], vec[0]) * 180.0f / 3.141592741012573f;
			return yaw < 0.0f ? yaw + 360.0f : yaw;
		}

		void vectoangles(const vec3& vec, vec3& angles)
		{
			float yaw;
			float pitch;

			if (vec[1] == 0.0f && vec[0] == 0.0f)
			{
				yaw = 0.0f;
				pitch = vec[2] > 0.0f ? 270.0f : 90.0f;
			}
			else
			{
				yaw = vectoyaw(vec);
				const float forward = std::sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
				pitch = std::atan2(-vec[2], forward) * 180.0f / 3.141592741012573f;
				if (pitch < 0.0f)
					pitch += 360.0f;
			}
			angles = { pitch, yaw, 0.0f };
		}

		// PM_ProjectVelocity: slides velocity along a plane while preserving its length, unless
		// that would speed the player up going downhill.
		void PM_ProjectVelocity(const vec3& velIn, const vec3& normal, vec3& velOut)
		{
			const float lengthSq2D = velIn[0] * velIn[0] + velIn[1] * velIn[1];

			if (std::fabs(normal[2]) < EQUAL_EPSILON || lengthSq2D == 0.0f)
			{
				velOut = velIn;
				return;
			}
			const float newZ = -(normal[0] * velIn[0] + normal[1] * velIn[1]) / normal[2];
			const float originalLengthSq = lengthSq2D + velIn[2] * velIn[2];
			const float adjustedLengthSq = lengthSq2D + newZ * newZ;
			const float lengthScale = std::sqrt(originalLengthSq / adjustedLengthSq);

			// The engine leaves velOut untouched when none of these hold.
			if (lengthScale < 1.0f || newZ < 0.0f || velIn[2] > 0.0f)
				velOut = vec3(velIn[0], velIn[1], newZ) * lengthScale;
		}

		uint16_t Trace_GetEntityHitId(const trace_t& trace)
		{
			if (trace.hitType == TRACE_HITTYPE_DYNENT_MODEL || trace.hitType == TRACE_HITTYPE_DYNENT_BRUSH)
				return ENTITYNUM_WORLD;
			if (trace.hitType == TRACE_HITTYPE_ENTITY)
				return trace.hitId;
			return ENTITYNUM_NONE;
		}

		int PM_GetEffectiveStance(const playerState_s* ps)
		{
			if (ps->viewHeightTarget == 22 || ps->viewHeightTarget == 40)
				return 2;
			return ps->viewHeightTarget == 11 ? 1 : 0;
		}

		bool PM_IsPlayerFrozenByWeapon(const playerState_s*)
		{
			// Weapon handling is not ported; nothing freezes the player mid-fire here.
			return false;
		}

		bool BG_UsingSniperScope(const playerState_s*)
		{
			// Weapon handling is not ported.
			return false;
		}

		int BG_GetMaxSprintTime(const playerState_s*)
		{
			// BG_GetWeaponDef(ps->weapon)->sprintDurationScale is 1 with no weapon asset loaded.
			const float maxSprintTime = DvarFloat("player_sprintTime", 4.0f) * 1000.0f;
			const int msec = static_cast<int>(maxSprintTime);
			return msec > 0x3FFF ? 0x3FFF : msec;
		}

		int PM_GetSprintLeft(const playerState_s* ps, int gametime)
		{
			const int maxSprintTime = BG_GetMaxSprintTime(ps);
			const SprintState& ss = ps->sprintState;
			int sprintLeft;

			if (ss.lastSprintStart)
			{
				if (ss.lastSprintStart <= ss.lastSprintEnd)
				{
					sprintLeft = gametime + ss.sprintStartMaxLength
						- (ss.lastSprintEnd - ss.lastSprintStart) - ss.lastSprintEnd;
					if (ss.sprintDelay)
						sprintLeft -= static_cast<int>(DvarFloat("player_sprintRechargePause", 0.0f) * 1000.0f);
				}
				else
				{
					sprintLeft = ss.sprintStartMaxLength - (gametime - ss.lastSprintStart);
				}
			}
			else
			{
				sprintLeft = maxSprintTime;
			}
			if (sprintLeft < 0)
				sprintLeft = 0;
			return maxSprintTime < sprintLeft ? maxSprintTime : sprintLeft;
		}

		void PM_ClearLadderFlag(playerState_s* ps)
		{
			if (ps->pm_flags & PMF_LADDER)
			{
				ps->pm_flags |= PMF_LADDER_FALL;
				ps->pm_flags &= ~PMF_LADDER;
			}
		}

		// --- bg_jump.cpp -----------------------------------------------------------------------

		float Jump_GetSlowdownFriction(const playerState_s* ps)
		{
			if (!DvarBool("jump_slowdownEnable", true))
				return 1.0f;
			if (ps->pm_time >= 1700)
				return 2.5f;
			return static_cast<float>(ps->pm_time * 1.5 * 0.0005882352706976235 + 1.0);
		}

		float Jump_GetLandFactor(const playerState_s* ps)
		{
			return Jump_GetSlowdownFriction(ps);
		}

		float Jump_ReduceFriction(playerState_s* ps)
		{
			if (ps->pm_time > JUMP_LAND_SLOWDOWN_TIME)
			{
				Jump_ClearState(ps);
				return 1.0f;
			}
			return Jump_GetSlowdownFriction(ps);
		}

		void Jump_ActivateSlowdown(playerState_s* ps)
		{
			if (!ps->pm_time)
			{
				ps->pm_flags |= PMF_JUMPING;
				ps->pm_time = JUMP_LAND_SLOWDOWN_TIME;
			}
		}

		void Jump_ApplySlowdown(playerState_s* ps)
		{
			float scale = 1.0f;

			if (ps->pm_time > JUMP_LAND_SLOWDOWN_TIME)
			{
				Jump_ClearState(ps);
				scale = 0.64999998f;
			}
			else if (!ps->pm_time)
			{
				if (ps->origin[2] >= ps->jumpOriginZ + 18.0f)
				{
					ps->pm_time = 1200;
					scale = 0.5f;
				}
				else
				{
					ps->pm_time = JUMP_LAND_SLOWDOWN_TIME;
					scale = 0.64999998f;
				}
			}
			if (DvarBool("jump_slowdownEnable", true))
				ps->velocity *= scale;
		}

		bool Jump_GetStepHeight(const playerState_s* ps, const vec3& origin, float* stepSize)
		{
			const float jumpHeight = DvarFloat("jump_height", 39.0f);

			if (origin[2] >= ps->jumpOriginZ + jumpHeight)
				return false;

			*stepSize = DvarFloat("jump_stepSize", 18.0f);
			if (ps->jumpOriginZ + jumpHeight < origin[2] + *stepSize)
				*stepSize = ps->jumpOriginZ + jumpHeight - origin[2];
			return true;
		}

		bool Jump_IsPlayerAboveMax(const playerState_s* ps)
		{
			return ps->origin[2] >= ps->jumpOriginZ + DvarFloat("jump_height", 39.0f);
		}

		void Jump_ClampVelocity(playerState_s* ps, const vec3& origin)
		{
			if (ps->origin[2] - origin[2] <= 0.0f)
				return;

			const float heightDiff = ps->jumpOriginZ + DvarFloat("jump_height", 39.0f) - ps->origin[2];
			if (heightDiff >= 0.1000000014901161f)
			{
				const float maxVel = std::sqrt(static_cast<float>(ps->gravity) * (heightDiff + heightDiff));
				if (maxVel < ps->velocity[2])
					ps->velocity[2] = maxVel;
			}
			else
			{
				ps->velocity[2] = 0.0f;
			}
		}

		void Jump_PushOffLadder(playerState_s* ps, pml_t* pml)
		{
			ps->velocity[2] *= 0.75f;

			vec3 flatForward(pml->forward[0], pml->forward[1], 0.0f);
			Vec3Normalize(flatForward);

			vec3 pushOffDir;
			if (glm::dot(ps->vLadderVec, pml->forward) >= 0.0f)
			{
				pushOffDir = flatForward;
			}
			else
			{
				const float dot = glm::dot(flatForward, ps->vLadderVec);
				pushOffDir = flatForward + ps->vLadderVec * (dot * -2.0f);
				Vec3Normalize(pushOffDir);
			}
			const float value = DvarFloat("jump_ladderPushVel", 128.0f);
			ps->velocity[0] = value * pushOffDir[0];
			ps->velocity[1] = value * pushOffDir[1];
			ps->pm_flags &= ~PMF_LADDER;
		}

		void Jump_Start(pmove_t* pm, pml_t* pml, float height)
		{
			playerState_s* ps = pm->ps;
			float velocitySqrd = (height + height) * static_cast<float>(ps->gravity);

			if ((ps->pm_flags & PMF_JUMPING) && ps->pm_time <= JUMP_LAND_SLOWDOWN_TIME)
				velocitySqrd /= Jump_GetLandFactor(ps);

			pml->groundPlane = 0;
			pml->almostGroundPlane = 0;
			pml->walking = 0;
			ps->groundEntityNum = ENTITYNUM_NONE;
			ps->jumpTime = pm->cmd.serverTime;
			ps->jumpOriginZ = ps->origin[2];
			ps->velocity[2] = std::sqrt(velocitySqrd);
			ps->pm_flags &= ~(PMF_TIME_HARDLANDING | PMF_TIME_KNOCKBACK);
			ps->pm_flags |= PMF_JUMPING;
			ps->pm_time = 0;
			ps->sprintState.sprintButtonUpRequired = 0;
			ps->aimSpreadScale += DvarFloat("jump_spreadAdd", 64.0f);
			if (ps->aimSpreadScale > 255.0f)
				ps->aimSpreadScale = 255.0f;
		}

		// --- speed scales ----------------------------------------------------------------------

		float PM_MoveScale(const playerState_s* ps, float fmove, float rmove, float umove)
		{
			float max = std::fabs(fmove);
			if (max < std::fabs(rmove))
				max = std::fabs(rmove);
			if (max < std::fabs(umove))
				max = std::fabs(umove);
			if (max == 0.0f)
				return 0.0f;

			const float total = std::sqrt(umove * umove + rmove * rmove + fmove * fmove);
			float scale = static_cast<float>(ps->speed) * max / (total * 127.0f);

			if ((ps->pm_flags & PMF_WALKING) || ps->leanf != 0.0f)
				scale *= 0.4000000059604645f;
			if (ps->pm_type == PM_NOCLIP)
				scale *= 3.0f;
			if (ps->pm_type == PM_UFO)
				scale *= 6.0f;
			if (ps->pm_type == PM_SPECTATOR)
				scale *= DvarFloat("player_spectateSpeedScale", 1.0f);
			return scale;
		}

		float PM_CmdScale(const playerState_s* ps, const usercmd_s* cmd)
		{
			const float total = std::sqrt(static_cast<float>(cmd->rightmove * cmd->rightmove
				+ cmd->forwardmove * cmd->forwardmove));

			int max = std::abs(static_cast<int>(cmd->forwardmove));
			if (std::abs(static_cast<int>(cmd->rightmove)) > max)
				max = std::abs(static_cast<int>(cmd->rightmove));
			if (!max)
				return 0.0f;

			float scale = static_cast<float>(ps->speed) * static_cast<float>(max) / (total * 127.0f);

			if ((ps->pm_flags & PMF_WALKING) || ps->leanf != 0.0f)
				scale *= 0.4000000059604645f;
			if (ps->pm_type == PM_NOCLIP)
				scale *= 3.0f;
			if (ps->pm_type == PM_UFO)
				scale *= 6.0f;
			if (ps->pm_type == PM_SPECTATOR)
				scale *= DvarFloat("player_spectateSpeedScale", 1.0f);
			return scale;
		}

		int PM_GetViewHeightLerpTime(const playerState_s*, int iTarget, int bDown)
		{
			if (iTarget == 11)
				return 400;
			if (iTarget != 40)
				return 200;
			return bDown ? 200 : 400;
		}

		float PM_GetViewHeightLerp(const pmove_t* pm, int iFromHeight, int iToHeight)
		{
			const playerState_s* ps = pm->ps;

			if (!ps->viewHeightLerpTime)
				return 0.0f;

			if (iFromHeight != -1 && iToHeight != -1
				&& (iToHeight != ps->viewHeightLerpTarget
					|| (iToHeight == 40
						&& (iFromHeight != 11 || ps->viewHeightLerpDown)
						&& (iFromHeight != 60 || !ps->viewHeightLerpDown))))
			{
				return 0.0f;
			}
			const float lerpFrac = static_cast<float>(pm->cmd.serverTime - ps->viewHeightLerpTime)
				/ static_cast<float>(PM_GetViewHeightLerpTime(ps, ps->viewHeightLerpTarget, ps->viewHeightLerpDown));

			if (lerpFrac < 0.0f)
				return 0.0f;
			return lerpFrac > 1.0f ? 1.0f : lerpFrac;
		}

		float PM_CmdScaleForStance(const pmove_t* pm)
		{
			const float toProne = PM_GetViewHeightLerp(pm, 40, 11);
			if (toProne != 0.0f)
				return toProne * 0.1500000059604645f + (1.0f - toProne) * 0.6499999761581421f;

			const float toCrouch = PM_GetViewHeightLerp(pm, 11, 40);
			if (toCrouch != 0.0f)
				return toCrouch * 0.6499999761581421f + (1.0f - toCrouch) * 0.1500000059604645f;

			switch (PM_GetEffectiveStance(pm->ps))
			{
			case 1: return 0.15000001f;
			case 2: return 0.64999998f;
			default: return 1.0f;
			}
		}

		float PM_CmdScale_Walk(const pmove_t* pm, const usercmd_s* cmd)
		{
			const playerState_s* ps = pm->ps;
			const bool proneAds = (ps->pm_flags & PMF_PRONE_BIT) != 0 && ps->fWeaponPosFrac > 0.0f;

			const float total = std::sqrt(static_cast<float>(cmd->rightmove * cmd->rightmove
				+ cmd->forwardmove * cmd->forwardmove));

			float fmove;
			if (cmd->forwardmove >= 0)
				fmove = std::fabs(static_cast<float>(cmd->forwardmove));
			else
				fmove = std::fabs(DvarFloat("player_backSpeedScale", 0.69999999f) * cmd->forwardmove);

			const float smove = std::fabs(DvarFloat("player_strafeSpeedScale", 0.80000001f) * cmd->rightmove);
			const float max = (fmove - smove) < 0.0f ? smove : fmove;
			if (max == 0.0f)
				return 0.0f;

			float scale = static_cast<float>(ps->speed) * max / (total * 127.0f);

			if ((ps->pm_flags & PMF_WALKING) || ps->leanf != 0.0f || proneAds)
				scale *= 0.4000000059604645f;
			if (ps->pm_flags & PMF_SPRINTING)
				scale *= DvarFloat("player_sprintSpeedScale", 1.5f);

			if (ps->pm_type == PM_NOCLIP)
				scale *= 3.0f;
			else if (ps->pm_type == PM_UFO)
				scale *= 6.0f;
			else
				scale *= PM_CmdScaleForStance(pm);

			// Weapon handling is not ported: moveSpeedScale/adsMoveSpeedScale and the shellshock
			// scale would multiply in here.
			return scale * ps->moveSpeedScaleMultiplier;
		}

		// --- inertia ---------------------------------------------------------------------------

		bool PM_DoPlayerInertia(const playerState_s* ps, float accelspeed, const vec3& wishdir)
		{
			const float velX = accelspeed * wishdir[0] + ps->velocity[0];
			const float velY = accelspeed * wishdir[1] + ps->velocity[1];
			const float oldX = ps->oldVelocity[0];
			const float oldY = ps->oldVelocity[1];

			const float oldVelLenSq = oldX * oldX + oldY * oldY;
			const float newVelLenSq = velX * velX + velY * velY;
			const float lenProduct = std::sqrt(newVelLenSq * oldVelLenSq);
			const float scaledDotAngle = velY * oldY + velX * oldX;

			return scaledDotAngle < DvarFloat("inertiaAngle", 0.0f) * lenProduct;
		}

		float PM_PlayerInertia(const playerState_s* ps, float accelspeed, const vec3& wishdir)
		{
			if (ps->pm_type == PM_NOCLIP)
				return accelspeed;

			const float inertiaMax = DvarFloat("inertiaMax", 50.0f);
			if (accelspeed <= inertiaMax)
				return accelspeed;

			const float oldSpeedSq = ps->oldVelocity[1] * ps->oldVelocity[1]
				+ ps->oldVelocity[0] * ps->oldVelocity[0];
			if (oldSpeedSq < 0.0001f)
				return accelspeed;

			return PM_DoPlayerInertia(ps, accelspeed, wishdir) ? inertiaMax : accelspeed;
		}

		// --- sprint ----------------------------------------------------------------------------

		bool PM_CanStand(playerState_s* ps, pmove_t* pm)
		{
			if ((ps->pm_flags & (PMF_PRONE_BIT | PMF_DUCKED)) == 0)
				return true;

			trace_t trace = {};
			const vec3 playerMins(-15.0f, -15.0f, 0.0f);
			const vec3 playerMaxs(15.0f, 15.0f, 70.0f);
			PM_PlayerTrace(pm, &trace, ps->origin, playerMins, playerMaxs, ps->origin,
				ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
			return !trace.allsolid;
		}

		bool PM_SprintStartInterferingButtons(const playerState_s* ps, int forwardSpeed, int buttons)
		{
			if (ps->pm_flags & PMF_LADDER)
				return true;
			if (forwardSpeed <= DvarInt("player_sprintForwardMinimum", 105))
				return true;
			if (buttons & 0xC435)
				return true;
			if (ps->leanf != 0.0f)
				return true;
			if (ps->pm_flags & (PMF_MANTLE | PMF_LADDER | PMF_SIGHT_AIMING | PMF_SHELLSHOCKED))
				return true;
			if ((ps->pm_flags & PMF_JUMPING) && !ps->pm_time)
				return false;

			return ps->weaponstate == 12 || ps->weaponstate == 13 || ps->weaponstate == 14
				|| (ps->weaponstate >= 15 && ps->weaponstate <= 20);
		}

		bool PM_SprintEndingButtons(const playerState_s* ps, int forwardSpeed, int buttons)
		{
			if (ps->pm_flags & (PMF_LADDER | PMF_SIGHT_AIMING | PMF_SHELLSHOCKED))
				return true;
			if (forwardSpeed <= DvarInt("player_sprintForwardMinimum", 105))
				return true;
			if (buttons & 0xC735)
				return true;
			if (ps->leanf != 0.0f)
				return true;

			return ps->weaponstate == 12 || ps->weaponstate == 13 || ps->weaponstate == 14
				|| (ps->weaponstate >= 15 && ps->weaponstate <= 20)
				|| ps->weaponstate == 25 || ps->weaponstate == 26;
		}

		void PM_StartSprint(playerState_s* ps, pmove_t* pm, int sprintLeft)
		{
			ps->sprintState.sprintStartMaxLength = sprintLeft;
			ps->sprintState.lastSprintStart = pm->cmd.serverTime;
			ps->pm_flags |= PMF_SPRINTING;
		}

		void PM_EndSprint(playerState_s* ps, pmove_t* pm)
		{
			if (ps->pm_flags & PMF_SPRINTING)
			{
				ps->sprintState.sprintDelay = 0;
				ps->sprintState.lastSprintEnd = pm->cmd.serverTime;
				ps->pm_flags &= ~PMF_SPRINTING;
				if (pm->cmd.buttons & BUTTON_SPRINT)
					ps->sprintState.sprintButtonUpRequired = 1;
			}
		}

		void PM_UpdateSprint(pmove_t* pm, const pml_t*)
		{
			playerState_s* ps = pm->ps;
			SprintState* ss = &ps->sprintState;

			if (ss->sprintButtonUpRequired && !(pm->cmd.buttons & BUTTON_SPRINT))
				ss->sprintButtonUpRequired = 0;

			if (ps->pm_type >= PM_NOCLIP || BG_GetMaxSprintTime(ps) <= 0)
			{
				PM_EndSprint(ps, pm);
				return;
			}
			if (ps->pm_flags & PMF_SPRINTING)
			{
				if (pm->cmd.serverTime - ss->lastSprintStart >= ss->sprintStartMaxLength)
				{
					PM_EndSprint(ps, pm);
					ss->sprintDelay = 1;
					return;
				}
				if (PM_SprintEndingButtons(ps, pm->cmd.forwardmove, pm->cmd.buttons))
				{
					PM_EndSprint(ps, pm);
					return;
				}
				if (!(pm->oldcmd.buttons & BUTTON_SPRINT) && (pm->cmd.buttons & BUTTON_SPRINT))
				{
					PM_EndSprint(ps, pm);
					ss->sprintButtonUpRequired = 1;
				}
			}
			else if ((!ss->sprintDelay
					|| DvarFloat("player_sprintRechargePause", 0.0f) * 1000.0f
						<= static_cast<float>(pm->cmd.serverTime - ss->lastSprintEnd))
				&& (pm->cmd.buttons & BUTTON_SPRINT)
				&& !(ps->pm_flags & PMF_NO_SPRINT)
				&& !ss->sprintButtonUpRequired
				&& !PM_SprintStartInterferingButtons(ps, pm->cmd.forwardmove, pm->cmd.buttons)
				&& PM_CanStand(ps, pm))
			{
				const int sprintLeft = PM_GetSprintLeft(ps, pm->cmd.serverTime);
				if (DvarFloat("player_sprintMinTime", 1.0f) * 1000.0f < static_cast<float>(sprintLeft))
					PM_StartSprint(ps, pm, sprintLeft);
			}
		}

		void PM_UpdatePlayerWalkingFlag(pmove_t* pm)
		{
			playerState_s* ps = pm->ps;

			ps->pm_flags &= ~PMF_WALKING;
			if (ps->pm_type < PM_DEAD
				&& (pm->cmd.buttons & CMD_BUTTON_ADS)
				&& !(ps->pm_flags & PMF_PRONE_BIT)
				&& (ps->pm_flags & PMF_SIGHT_AIMING)
				&& ps->weaponstate != 7 && ps->weaponstate != 9 && ps->weaponstate != 11
				&& ps->weaponstate != 10 && ps->weaponstate != 8)
			{
				ps->pm_flags |= PMF_WALKING;
			}
		}

		// The engine gates ads on the weapon def, which is not ported: with no weapon nothing is
		// ever ads-capable, so the flag only ever clears.
		void PM_UpdateAimDownSightFlag(pmove_t* pm, pml_t*)
		{
			pm->ps->pm_flags &= ~PMF_SIGHT_AIMING;
		}

		void PM_DropTimers(playerState_s* ps, pml_t* pml)
		{
			if (ps->pm_time)
			{
				if (pml->msec < ps->pm_time)
				{
					ps->pm_time -= pml->msec;
				}
				else
				{
					if (ps->pm_flags & PMF_JUMPING)
						Jump_ClearState(ps);
					ps->pm_flags &= ~(PMF_TIME_HARDLANDING | PMF_TIME_KNOCKBACK | PMF_JUMPING);
					ps->pm_time = 0;
				}
			}
			if (ps->legsTimer > 0)
			{
				ps->legsTimer -= pml->msec;
				if (ps->legsTimer < 0)
					ps->legsTimer = 0;
			}
			if (ps->torsoTimer > 0)
			{
				ps->torsoTimer -= pml->msec;
				if (ps->torsoTimer < 0)
					ps->torsoTimer = 0;
			}
		}

		void PM_MeleeChargeClear(playerState_s* ps)
		{
			ps->pm_flags &= ~PMF_MELEE_CHARGE;
			ps->meleeChargeYaw = 0.0f;
			ps->meleeChargeDist = 0;
			ps->meleeChargeTime = 0;
		}

		void PM_MeleeChargeUpdate(pmove_t* pm, pml_t* pml)
		{
			playerState_s* ps = pm->ps;
			const bool chargeValid = (ps->pm_flags & PMF_MELEE_CHARGE)
				&& ps->pm_type == PM_NORMAL
				&& !(ps->eFlags & EF_TURRET_ACTIVE)
				&& !(ps->pm_flags & (PMF_MANTLE | PMF_LADDER));

			if (!chargeValid)
			{
				PM_MeleeChargeClear(ps);
				return;
			}
			const float chargeFriction = DvarFloat("player_meleeChargeFriction", 1200.0f);
			if (!ps->meleeChargeTime)
			{
				const float yaw = ps->meleeChargeYaw * DEG_TO_RAD;
				const float chargeVel = std::sqrt(chargeFriction
					* static_cast<float>(ps->meleeChargeDist + ps->meleeChargeDist));

				ps->velocity[0] = chargeVel * std::cos(yaw);
				ps->velocity[1] = chargeVel * std::sin(yaw);
				ps->meleeChargeTime = static_cast<int>(chargeVel / chargeFriction * 1000.0f);
			}
			ps->meleeChargeTime -= pml->msec;
			if (ps->meleeChargeTime <= 0)
				PM_MeleeChargeClear(ps);
		}

		// --- view height -----------------------------------------------------------------------

		float PM_ViewHeightTableLerp(int iFrac, const viewLerpWaypoint_s* pTable, float* pfPosOfs)
		{
			if (!iFrac)
			{
				*pfPosOfs = static_cast<float>(pTable->iOffset);
				return pTable->fViewHeight;
			}
			for (int i = 1; pTable[i].iFrac != -1; ++i)
			{
				const viewLerpWaypoint_s* pCurr = &pTable[i];
				if (iFrac == pCurr->iFrac)
				{
					*pfPosOfs = static_cast<float>(pCurr->iOffset);
					return pCurr->fViewHeight;
				}
				if (pCurr->iFrac > iFrac)
				{
					const viewLerpWaypoint_s* pPrev = &pTable[i - 1];
					const float entryFrac = static_cast<float>(iFrac - pPrev->iFrac)
						/ static_cast<float>(pCurr->iFrac - pPrev->iFrac);
					*pfPosOfs = pPrev->iOffset + (pCurr->iOffset - pPrev->iOffset) * entryFrac;
					return (pCurr->fViewHeight - pPrev->fViewHeight) * entryFrac + pPrev->fViewHeight;
				}
			}
			*pfPosOfs = static_cast<float>(pTable->iOffset);
			return pTable->fViewHeight;
		}

		float PM_ViewHeightTableForTarget(const playerState_s* ps, int iLerpFrac, float* pfPosOfs)
		{
			if (ps->viewHeightLerpTarget == 11)
				return PM_ViewHeightTableLerp(iLerpFrac, viewLerp_CrouchProne, pfPosOfs);
			if (ps->viewHeightLerpTarget == 40)
			{
				return ps->viewHeightLerpDown
					? PM_ViewHeightTableLerp(iLerpFrac, viewLerp_StandCrouch, pfPosOfs)
					: PM_ViewHeightTableLerp(iLerpFrac, viewLerp_ProneCrouch, pfPosOfs);
			}
			return PM_ViewHeightTableLerp(iLerpFrac, viewLerp_CrouchStand, pfPosOfs);
		}

		void PM_ViewHeightAdjust(pmove_t* pm, pml_t* pml)
		{
			playerState_s* ps = pm->ps;

			if (!ps->viewHeightTarget || ps->viewHeightCurrent == 0.0f)
			{
				if (ps->pm_type == PM_SPECTATOR)
					ps->viewHeightCurrent = 0.0f;
				else
					ps->viewHeightCurrent = static_cast<float>(ps->viewHeightTarget);
				return;
			}
			if (ps->viewHeightCurrent == static_cast<float>(ps->viewHeightTarget) && !ps->viewHeightLerpTime)
				return;

			// Anything outside the three stance heights slides at a flat 180 units per second.
			if (ps->viewHeightTarget != 11 && ps->viewHeightTarget != 40 && ps->viewHeightTarget != 60)
			{
				ps->viewHeightLerpTime = 0;
				if (ps->viewHeightCurrent >= static_cast<float>(ps->viewHeightTarget))
				{
					ps->viewHeightCurrent -= pml->frametime * 180.0f;
					if (ps->viewHeightCurrent <= static_cast<float>(ps->viewHeightTarget))
						ps->viewHeightCurrent = static_cast<float>(ps->viewHeightTarget);
				}
				else
				{
					ps->viewHeightCurrent += pml->frametime * 180.0f;
					if (ps->viewHeightCurrent >= static_cast<float>(ps->viewHeightTarget))
						ps->viewHeightCurrent = static_cast<float>(ps->viewHeightTarget);
				}
				return;
			}
			int iLerpFrac = 0;
			float posOfs = 0.0f;

			if (ps->viewHeightLerpTime)
			{
				const int lerpTime = PM_GetViewHeightLerpTime(ps, ps->viewHeightLerpTarget,
					ps->viewHeightLerpDown);
				iLerpFrac = 100 * (pm->cmd.serverTime - ps->viewHeightLerpTime) / lerpTime;
				if (iLerpFrac < 0)
					iLerpFrac = 0;
				else if (iLerpFrac > 100)
					iLerpFrac = 100;

				if (iLerpFrac == 100)
				{
					ps->viewHeightCurrent = static_cast<float>(ps->viewHeightLerpTarget);
					ps->viewHeightLerpTime = 0;
				}
				else
				{
					ps->viewHeightCurrent = PM_ViewHeightTableForTarget(ps, iLerpFrac, &posOfs);
				}
			}
			if (ps->viewHeightLerpTime)
			{
				// A stance change part way through a lerp reverses it from where it got to.
				if (ps->viewHeightTarget != ps->viewHeightLerpTarget
					&& ((ps->viewHeightTarget < ps->viewHeightLerpTarget && !ps->viewHeightLerpDown)
						|| (ps->viewHeightTarget > ps->viewHeightLerpTarget && ps->viewHeightLerpDown)))
				{
					iLerpFrac = 100 - iLerpFrac;
					ps->viewHeightLerpDown ^= 1;

					if (ps->viewHeightLerpDown)
					{
						if (ps->viewHeightLerpTarget == 60)
							ps->viewHeightLerpTarget = 40;
						else if (ps->viewHeightLerpTarget == 40)
							ps->viewHeightLerpTarget = 11;
					}
					else if (ps->viewHeightLerpTarget == 11)
					{
						ps->viewHeightLerpTarget = 40;
					}
					else if (ps->viewHeightLerpTarget == 40)
					{
						ps->viewHeightLerpTarget = 60;
					}

					if (iLerpFrac == 100)
					{
						ps->viewHeightCurrent = static_cast<float>(ps->viewHeightLerpTarget);
						ps->viewHeightLerpTime = 0;
					}
					else
					{
						const int lerpTime = PM_GetViewHeightLerpTime(ps, ps->viewHeightLerpTarget,
							ps->viewHeightLerpDown);
						ps->viewHeightLerpTime = pm->cmd.serverTime
							- static_cast<int>(iLerpFrac * 0.009999999776482582f * lerpTime);
						PM_ViewHeightTableForTarget(ps, iLerpFrac, &posOfs);
					}
				}
			}
			else if (ps->viewHeightCurrent != static_cast<float>(ps->viewHeightTarget))
			{
				ps->viewHeightLerpTime = pm->cmd.serverTime;
				switch (ps->viewHeightTarget)
				{
				case 11:
					ps->viewHeightLerpDown = 1;
					ps->viewHeightLerpTarget = ps->viewHeightCurrent <= 40.0f ? 11 : 40;
					break;
				case 40:
					ps->viewHeightLerpDown = ps->viewHeightCurrent > static_cast<float>(ps->viewHeightTarget);
					ps->viewHeightLerpTarget = 40;
					break;
				case 60:
					ps->viewHeightLerpDown = 0;
					ps->viewHeightLerpTarget = ps->viewHeightCurrent >= 40.0f ? 60 : 40;
					break;
				default:
					break;
				}
			}
		}

		// BG_CheckProne is not ported: the flat world lets a grounded player go prone anywhere.
		bool PlayerProneAllowed(pmove_t* pm)
		{
			playerState_s* ps = pm->ps;

			if (ps->pm_flags & PMF_PRONE_BIT)
				return true;
			return ps->groundEntityNum != ENTITYNUM_NONE;
		}

		void PM_CheckDuck(pmove_t* pm, pml_t* pml)
		{
			playerState_s* ps = pm->ps;
			trace_t trace = {};

			pm->proneChange = 0;

			if (ps->pm_type == PM_SPECTATOR)
			{
				pm->mins = { -8.0f, -8.0f, -8.0f };
				pm->maxs = { 8.0f, 8.0f, 16.0f };
				ps->pm_flags &= ~(PMF_PRONE_BIT | PMF_DUCKED);
				pm->cmd.buttons &= ~CMD_BUTTON_PRONE;
				ps->viewHeightTarget = 0;
				ps->viewHeightCurrent = 0.0f;
				return;
			}
			const bool bWasProne = (ps->pm_flags & PMF_PRONE_BIT) != 0;

			pm->mins = { -15.0f, -15.0f, 0.0f };
			pm->maxs = { 15.0f, 15.0f, 70.0f };

			if (ps->pm_type == PM_DEAD)
			{
				ps->viewHeightTarget = 8;
				PM_ViewHeightAdjust(pm, pml);
				return;
			}
			if (ps->pm_flags & PMF_VEHICLE_ATTACHED)
			{
				ps->viewHeightTarget = 60;
				ps->pm_flags &= ~(PMF_PRONE_BIT | PMF_DUCKED);
				PM_ViewHeightAdjust(pm, pml);
				return;
			}
			if (ps->pm_flags & PMF_SPRINTING)
			{
				ps->viewHeightTarget = 60;
				ps->eFlags &= ~(EF_CROUCH | EF_PRONE);
				ps->pm_flags &= ~(PMF_PRONE_BIT | PMF_DUCKED);
				PM_ViewHeightAdjust(pm, pml);
				return;
			}
			if (ps->eFlags & EF_TURRET_ACTIVE)
			{
				if ((ps->eFlags & 0x100) && !(ps->eFlags & 0x200))
				{
					ps->pm_flags |= PMF_PRONE_BIT;
					ps->pm_flags &= ~PMF_DUCKED;
				}
				else if ((ps->eFlags & 0x200) && !(ps->eFlags & 0x100))
				{
					ps->pm_flags |= PMF_DUCKED;
					ps->pm_flags &= ~PMF_PRONE_BIT;
				}
				else
				{
					ps->pm_flags &= ~(PMF_PRONE_BIT | PMF_DUCKED);
				}
			}
			else if (!(ps->pm_flags & (PMF_RESPAWNED | PMF_FROZEN)) && !PM_IsPlayerFrozenByWeapon(ps))
			{
				if (ps->pm_type == PM_LASTSTAND)
				{
					ps->pm_flags &= ~PMF_PRONE_BIT;
					ps->pm_flags |= PMF_DUCKED;
				}
				else
				{
					if ((ps->pm_flags & PMF_LADDER) && (pm->cmd.buttons & CMD_BUTTON_STANCE))
						pm->cmd.buttons &= ~CMD_BUTTON_STANCE;

					if (!(pm->cmd.buttons & CMD_BUTTON_PRONE) || (ps->pm_flags & PMF_RESPAWNED))
					{
						if (pm->cmd.buttons & CMD_BUTTON_CROUCH)
						{
							if (ps->pm_flags & PMF_PRONE_BIT)
							{
								pm->maxs[2] = 50.0f;
								PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
									ps->origin, ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
								if (!trace.allsolid)
								{
									ps->pm_flags &= ~PMF_PRONE_BIT;
									ps->pm_flags |= PMF_DUCKED;
								}
							}
							else
							{
								ps->pm_flags |= PMF_DUCKED;
							}
						}
						else if (ps->pm_flags & PMF_PRONE_BIT)
						{
							PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
								ps->origin, ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
							if (trace.allsolid)
							{
								pm->maxs[2] = 50.0f;
								PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
									ps->origin, ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
								if (!trace.allsolid)
								{
									ps->pm_flags &= ~PMF_PRONE_BIT;
									ps->pm_flags |= PMF_DUCKED;
								}
							}
							else
							{
								ps->pm_flags &= ~(PMF_PRONE_BIT | PMF_DUCKED);
							}
						}
						else if (ps->pm_flags & PMF_DUCKED)
						{
							PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
								ps->origin, ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
							if (!trace.allsolid)
								ps->pm_flags &= ~PMF_DUCKED;
						}
					}
					else if (PlayerProneAllowed(pm))
					{
						ps->pm_flags |= PMF_PRONE_BIT;
						ps->pm_flags &= ~PMF_DUCKED;
					}
					else if (ps->groundEntityNum != ENTITYNUM_NONE)
					{
						ps->pm_flags |= PMF_NO_PRONE;
					}
				}
			}
			if (!ps->viewHeightLerpTime)
			{
				if (ps->pm_type == PM_LASTSTAND)
				{
					ps->viewHeightTarget = 22;
				}
				else if (ps->pm_flags & PMF_PRONE_BIT)
				{
					if (ps->viewHeightTarget == 60)
					{
						ps->viewHeightTarget = 40;
					}
					else if (ps->viewHeightTarget != 11)
					{
						ps->viewHeightTarget = 11;
						pm->proneChange = 1;
						Jump_ActivateSlowdown(ps);
					}
				}
				else if (ps->viewHeightTarget == 11)
				{
					ps->viewHeightTarget = 40;
					pm->proneChange = 1;
				}
				else if (ps->pm_flags & PMF_DUCKED)
				{
					ps->viewHeightTarget = 40;
				}
				else
				{
					ps->viewHeightTarget = 60;
				}
			}
			PM_ViewHeightAdjust(pm, pml);

			switch (PM_GetEffectiveStance(ps))
			{
			case 1:
				pm->maxs[2] = 30.0f;
				ps->eFlags |= EF_PRONE;
				ps->eFlags &= ~EF_CROUCH;
				ps->pm_flags |= PMF_PRONE_BIT;
				ps->pm_flags &= ~PMF_DUCKED;
				break;
			case 2:
				pm->maxs[2] = 50.0f;
				ps->eFlags |= EF_CROUCH;
				ps->eFlags &= ~EF_PRONE;
				ps->pm_flags |= PMF_DUCKED;
				ps->pm_flags &= ~PMF_PRONE_BIT;
				break;
			default:
				pm->maxs[2] = 70.0f;
				ps->eFlags &= ~(EF_CROUCH | EF_PRONE);
				ps->pm_flags &= ~(PMF_PRONE_BIT | PMF_DUCKED);
				break;
			}

			if ((ps->pm_flags & PMF_PRONE_BIT) && !bWasProne)
			{
				if (pm->cmd.forwardmove || pm->cmd.rightmove)
					ps->pm_flags &= ~PMF_PRONEMOVE_OVERRIDDEN;

				// Going prone lifts the player clear of the floor and drops them back, which can
				// move the origin.
				vec3 vEnd = ps->origin;
				vEnd[2] += 10.0f;
				PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, vEnd,
					ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
				vEnd = Vec3Lerp(ps->origin, vEnd, trace.fraction);
				PM_PlayerTrace(pm, &trace, vEnd, pm->mins, pm->maxs, ps->origin,
					ps->clientNum, pm->tracemask & ~CONTENTS_BODY);
				ps->origin = Vec3Lerp(vEnd, ps->origin, trace.fraction);
				ps->proneDirection = ps->viewangles[1];
				ps->proneDirectionPitch = 0.0f;
				ps->proneTorsoPitch = ps->proneDirectionPitch;
			}
		}

		// --- view angles -----------------------------------------------------------------------

		// Only the clamp half of PM_UpdateViewAngles is ported; lean, prone yaw/pitch clamps and
		// the ladder clamp are not.
		void PM_UpdateViewAngles(playerState_s* ps, float, usercmd_s* cmd, char)
		{
			if (ps->pm_type == PM_INTERMISSION || ps->pm_type >= PM_DEAD)
				return;

			const float minPitch = DvarFloat("player_view_pitch_up", 85.0f);
			const float maxPitch = DvarFloat("player_view_pitch_down", 85.0f);

			for (int i = 0; i < 3; ++i)
			{
				const float raw = ps->delta_angles[i] + cmd->angles[i] * SHORT_TO_DEGREE;
				const float scaled = raw * ONE_OVER_360;
				float temp = (scaled - std::floor(scaled + 0.5f)) * 360.0f;

				if (i == 0)
				{
					if (temp > maxPitch)
					{
						ps->delta_angles[0] = maxPitch - cmd->angles[0] * SHORT_TO_DEGREE;
						temp = maxPitch;
					}
					else if (temp < -minPitch)
					{
						ps->delta_angles[0] = -minPitch - cmd->angles[0] * SHORT_TO_DEGREE;
						temp = -minPitch;
					}
				}
				const float normalized = temp * ONE_OVER_360;
				ps->viewangles[i] = (normalized - std::floor(normalized + 0.5f)) * 360.0f;
			}
		}

		void PM_SetMovementDir(pmove_t* pm, pml_t* pml)
		{
			playerState_s* ps = pm->ps;
			int moveyaw;

			if ((ps->pm_flags & PMF_PRONE_BIT) && !(ps->eFlags & EF_TURRET_ACTIVE))
			{
				moveyaw = static_cast<int>(AngleDelta(ps->proneDirection, ps->viewangles[1]));
			}
			else if (ps->pm_flags & PMF_LADDER)
			{
				moveyaw = static_cast<int>(AngleDelta(vectoyaw(ps->vLadderVec) + 180.0f, ps->viewangles[1]));
			}
			else
			{
				const vec3 moved = ps->origin - pml->previous_origin;
				const float speed = std::sqrt(glm::dot(moved, moved));

				if ((!pm->cmd.forwardmove && !pm->cmd.rightmove)
					|| ps->groundEntityNum == ENTITYNUM_NONE
					|| speed == 0.0f
					|| speed <= pml->frametime * 5.0f)
				{
					ps->movementDir = 0;
					return;
				}
				vec3 dir;
				Vec3NormalizeTo(moved, dir);
				vectoangles(dir, dir);
				moveyaw = static_cast<int>(AngleDelta(dir[1], ps->viewangles[1]));

				if (pm->cmd.forwardmove < 0)
				{
					const float flipped = (moveyaw + 180.0f) * ONE_OVER_360;
					moveyaw = static_cast<int>((flipped - std::floor(flipped + 0.5f)) * 360.0f);
				}
			}
			if (std::abs(moveyaw) > 90)
				moveyaw = moveyaw <= 0 ? -90 : 90;
			ps->movementDir = static_cast<char>(moveyaw);
		}

		float PM_PermuteRestrictiveClipPlanes(const vec3& velocity, int planeCount, const vec3* planes,
			int* permutation)
		{
			float parallel[8];

			for (int planeIndex = 0; planeIndex < planeCount; ++planeIndex)
			{
				parallel[planeIndex] = glm::dot(velocity, planes[planeIndex]);

				int permutedIndex = planeIndex;
				while (permutedIndex > 0 && parallel[planeIndex] <= parallel[permutation[permutedIndex - 1]])
				{
					permutation[permutedIndex] = permutation[permutedIndex - 1];
					--permutedIndex;
				}
				permutation[permutedIndex] = planeIndex;
			}
			return parallel[permutation[0]];
		}

		// BG_CheckProne is not ported, so a prone position is always accepted and the fallback
		// origin/velocity are never restored.
		bool PM_VerifyPronePosition(pmove_t*, const vec3&, const vec3&)
		{
			return true;
		}

		void PM_DeadMove(playerState_s* ps, pml_t* pml)
		{
			if (!pml->walking)
				return;

			const float forward = std::sqrt(glm::dot(ps->velocity, ps->velocity)) - 20.0f;
			if (forward > 0.0f)
			{
				Vec3Normalize(ps->velocity);
				ps->velocity *= forward;
			}
			else
			{
				ps->velocity = vec3(0.0f);
			}
		}

		void PM_NoclipMove(pmove_t* pm, pml_t* pml)
		{
			playerState_s* ps = pm->ps;

			ps->viewHeightTarget = 60;

			const float speed = std::sqrt(glm::dot(ps->velocity, ps->velocity));
			if (speed >= 1.0f)
			{
				const float stopspeed = DvarFloat("stopspeed", 100.0f);
				const float control = stopspeed <= speed ? speed : stopspeed;
				const float drop = control * (DvarFloat("friction", 5.5f) * 1.5f) * pml->frametime;

				float newspeed = speed - drop;
				if (newspeed < 0.0f)
					newspeed = 0.0f;
				ps->velocity *= newspeed / speed;
			}
			else
			{
				ps->velocity = vec3(0.0f);
			}
			const float fmove = static_cast<float>(pm->cmd.forwardmove);
			const float smove = static_cast<float>(pm->cmd.rightmove);
			float umove = 0.0f;
			if (pm->cmd.buttons & 0x80)
				umove += 127.0f;
			if (pm->cmd.buttons & 0x40)
				umove -= 127.0f;

			const float scale = PM_MoveScale(ps, fmove, smove, umove);
			vec3 wishvel = pml->forward * fmove + pml->right * smove + pml->up * umove;

			vec3 wishdir = wishvel;
			const float wishspeed = Vec3Normalize(wishdir) * scale;

			PM_Accelerate(ps, pml, wishdir, wishspeed, 9.0f);
			ps->origin += ps->velocity * pml->frametime;
		}

		void PM_FlyMove(pmove_t* pm, pml_t* pml)
		{
			playerState_s* ps = pm->ps;

			PM_Friction(ps, pml);

			float scale = PM_CmdScale(ps, &pm->cmd);
			vec3 wishvel(0.0f);

			if (scale != 0.0f)
			{
				const vec3 up(0.0f, 0.0f, 1.0f);
				const vec3 forward = glm::cross(up, pml->right);
				wishvel = (forward * static_cast<float>(pm->cmd.forwardmove)
					+ pml->right * static_cast<float>(pm->cmd.rightmove)) * scale;
			}
			if (ps->speed)
			{
				scale = PM_MoveScale(ps, 0.0f, 0.0f, 127.0f);
				if (pm->cmd.buttons & 0x40)
					wishvel[2] -= scale * 127.0f;
				if (pm->cmd.buttons & 0x80)
					wishvel[2] += scale * 127.0f;
			}
			vec3 wishdir = wishvel;
			const float wishspeed = Vec3Normalize(wishdir);

			PM_Accelerate(ps, pml, wishdir, wishspeed, 8.0f);
			PM_StepSlideMove(pm, pml, false);
		}

		// Movement mode dispatch. Only these three calls are swapped, exactly as
		// src/Game/Player/PMove.cpp does it; every other call in PmoveSingle stays the engine's.
		void WalkMove(pmove_t* pm, pml_t* pml)
		{
			switch (g_mode)
			{
			case Mode::COD4: PM_WalkMove(pm, pml); break;
			case Mode::Q3: Q3::WalkMove(pm, pml, false); break;
			case Mode::Q3CPM: Q3::WalkMove(pm, pml, true); break;
			case Mode::CS: CS::WalkMove(pm, pml); break;
			}
		}

		void AirMove(pmove_t* pm, pml_t* pml)
		{
			switch (g_mode)
			{
			case Mode::COD4: PM_AirMove(pm, pml); break;
			case Mode::Q3: Q3::AirMove(pm, pml, false); break;
			case Mode::Q3CPM: Q3::AirMove(pm, pml, true); break;
			case Mode::CS: CS::AirMove(pm, pml); break;
			}
		}

		void GroundTrace(pmove_t* pm, pml_t* pml)
		{
			switch (g_mode)
			{
			case Mode::COD4: PM_GroundTrace(pm, pml); break;
			case Mode::Q3:
			case Mode::Q3CPM: Q3::GroundTrace(pm, pml); break;
			case Mode::CS: CS::GroundTrace(pm, pml); break;
			}
		}
	}

	void SetMode(Mode mode)
	{
		g_mode = mode;
	}

	Mode GetMode()
	{
		return g_mode;
	}

	// The engine's clip has no Q3 overbounce divide: it backs the parallel component off by a
	// fixed fraction of its own magnitude. overbounce arrives as 1 + that fraction, and the
	// canonical 1.001f maps straight onto EQUAL_EPSILON so the engine path keeps its own constant.
	void PM_ClipVelocity(const vec3& in, const vec3& normal, vec3& out, float overbounce)
	{
		const float backoff = overbounce == PM_OVERCLIP ? EQUAL_EPSILON : overbounce - 1.0f;

		float parallel = glm::dot(in, normal);
		parallel -= std::fabs(parallel) * backoff;
		out = in - normal * parallel;
	}

	void PM_Friction(playerState_s* ps, pml_t* pml)
	{
		vec3 vec = ps->velocity;
		if (pml->walking)
			vec[2] = 0.0f;

		const float speed = std::sqrt(glm::dot(vec, vec));
		if (speed < 1.0f)
		{
			ps->velocity = vec3(0.0f);
			return;
		}
		float drop = 0.0f;

		if (ps->pm_flags & PMF_MELEE_CHARGE)
		{
			drop = DvarFloat("player_meleeChargeFriction", 1200.0f) * pml->frametime;
		}
		else if (pml->walking && !(pml->groundTrace.surfaceFlags & SURF_SLICK)
			&& !(ps->pm_flags & PMF_TIME_KNOCKBACK))
		{
			const float stopspeed = DvarFloat("stopspeed", 100.0f);
			float control = stopspeed <= speed ? speed : stopspeed;

			if (ps->pm_flags & PMF_TIME_HARDLANDING)
				control *= 0.300000011920929f;
			else if (ps->pm_flags & PMF_JUMPING)
				control *= Jump_ReduceFriction(ps);

			drop += control * DvarFloat("friction", 5.5f) * pml->frametime;
		}
		if (ps->pm_type == PM_SPECTATOR)
			drop += speed * 5.0f * pml->frametime;

		float newspeed = speed - drop;
		if (newspeed < 0.0f)
			newspeed = 0.0f;
		ps->velocity *= newspeed / speed;
	}

	void PM_Accelerate(playerState_s* ps, const pml_t* pml, const vec3& wishdir, float wishspeed, float accel)
	{
		if (ps->pm_flags & PMF_LADDER)
		{
			vec3 pushDir = wishdir * wishspeed - ps->velocity;
			const float pushLen = Vec3Normalize(pushDir);

			float canPush = accel * pml->frametime * wishspeed;
			if (pushLen < canPush)
				canPush = pushLen;

			ps->velocity += pushDir * canPush;
			return;
		}
		const float currentspeed = glm::dot(ps->velocity, wishdir);
		const float addspeed = wishspeed - currentspeed;
		if (addspeed <= 0.0f)
			return;

		// The engine accelerates toward at least stopspeed, so a tiny wishspeed still ramps fast.
		const float stopspeed = DvarFloat("stopspeed", 100.0f);
		const float control = stopspeed <= wishspeed ? wishspeed : stopspeed;

		float accelspeed = accel * pml->frametime * control;
		if (addspeed < accelspeed)
			accelspeed = addspeed;

		ps->velocity += wishdir * PM_PlayerInertia(ps, accelspeed, wishdir);
	}

	bool PM_SlideMove(pmove_t* pm, pml_t* pml, bool gravity)
	{
		playerState_s* ps = pm->ps;
		const int numbumps = 4;

		vec3 planes[8];
		int permutation[8] = {};
		trace_t trace = {};

		vec3 primal_velocity = ps->velocity;
		vec3 endVelocity = ps->velocity;

		if (gravity)
		{
			endVelocity[2] -= static_cast<float>(ps->gravity) * pml->frametime;
			ps->velocity[2] = (ps->velocity[2] + endVelocity[2]) * 0.5f;
			primal_velocity[2] = endVelocity[2];

			if (pml->groundPlane)
				PM_ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity, PM_OVERCLIP);
		}
		float time_left = pml->frametime;
		int numplanes = 0;

		if (pml->groundPlane)
		{
			planes[0] = pml->groundTrace.normal;
			numplanes = 1;
		}
		Vec3NormalizeTo(ps->velocity, planes[numplanes]);
		++numplanes;

		int bumpcount;
		for (bumpcount = 0; bumpcount < numbumps; ++bumpcount)
		{
			const vec3 end = ps->origin + ps->velocity * time_left;
			PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

			if (trace.allsolid)
			{
				ps->velocity[2] = 0.0f;
				return true;
			}
			if (trace.fraction > 0.0f)
				ps->origin = Vec3Lerp(ps->origin, end, trace.fraction);
			if (trace.fraction == 1.0f)
				break;

			PM_AddTouchEnt(pm, Trace_GetEntityHitId(trace));
			time_left -= time_left * trace.fraction;

			if (numplanes >= 8)
			{
				ps->velocity = vec3(0.0f);
				return true;
			}
			int i;
			for (i = 0; i < numplanes; ++i)
			{
				if (glm::dot(trace.normal, planes[i]) > 0.9990000128746033f)
				{
					PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity, PM_OVERCLIP);
					ps->velocity += trace.normal;
					break;
				}
			}
			if (i < numplanes)
				continue;

			planes[numplanes] = trace.normal;
			++numplanes;

			const float into = PM_PermuteRestrictiveClipPlanes(ps->velocity, numplanes, planes, permutation);
			if (into >= 0.1000000014901161f)
				continue;

			if (pml->impactSpeed < -into)
				pml->impactSpeed = -into;

			vec3 clipVelocity;
			vec3 endClipVelocity;
			PM_ClipVelocity(ps->velocity, planes[permutation[0]], clipVelocity, PM_OVERCLIP);
			PM_ClipVelocity(endVelocity, planes[permutation[0]], endClipVelocity, PM_OVERCLIP);

			for (int j = 1; j < numplanes; ++j)
			{
				if (glm::dot(clipVelocity, planes[permutation[j]]) >= 0.1000000014901161f)
					continue;

				PM_ClipVelocity(clipVelocity, planes[permutation[j]], clipVelocity, PM_OVERCLIP);
				PM_ClipVelocity(endClipVelocity, planes[permutation[j]], endClipVelocity, PM_OVERCLIP);

				if (glm::dot(clipVelocity, planes[permutation[0]]) >= 0.0f)
					continue;

				vec3 dir = glm::cross(planes[permutation[0]], planes[permutation[j]]);
				Vec3Normalize(dir);

				clipVelocity = dir * glm::dot(dir, ps->velocity);
				endClipVelocity = dir * glm::dot(dir, endVelocity);

				for (int k = 1; k < numplanes; ++k)
				{
					if (k != j && glm::dot(clipVelocity, planes[permutation[k]]) < 0.1000000014901161f)
					{
						ps->velocity = vec3(0.0f);
						return true;
					}
				}
			}
			ps->velocity = clipVelocity;
			endVelocity = endClipVelocity;
		}
		if (gravity)
			ps->velocity = endVelocity;

		// Any active pm_time -- a landing or a knockback -- hands the whole move's velocity back.
		if (ps->pm_time)
			ps->velocity = primal_velocity;

		return bumpcount != 0;
	}

	void PM_StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity)
	{
		playerState_s* ps = pm->ps;
		trace_t trace = {};

		float fStepAmount = 0.0f;
		bool jumping = false;
		bool bHadGround;

		if (ps->pm_flags & PMF_LADDER)
		{
			bHadGround = false;
			Jump_ClearState(ps);
		}
		else if (pml->groundPlane)
		{
			bHadGround = true;
		}
		else
		{
			bHadGround = false;
			if ((ps->pm_flags & PMF_JUMPING) && ps->pm_time)
				Jump_ClearState(ps);
		}
		const vec3 start_o = ps->origin;
		const vec3 start_v = ps->velocity;

		const bool iBumps = PM_SlideMove(pm, pml, gravity);

		float fStepSize = (ps->pm_flags & PMF_PRONE_BIT) ? 10.0f : 18.0f;

		if (ps->groundEntityNum == ENTITYNUM_NONE)
		{
			if ((ps->pm_flags & PMF_JUMPING) && ps->pm_time)
				Jump_ClearState(ps);

			if (iBumps && (ps->pm_flags & PMF_JUMPING) && Jump_GetStepHeight(ps, start_o, &fStepSize))
			{
				if (fStepSize < 1.0f)
					return;
				jumping = true;
			}
			if (!jumping && !((ps->pm_flags & PMF_LADDER) && ps->velocity[2] > 0.0f))
				return;
		}
		const vec3 down_o = ps->origin;
		const vec3 down_v = ps->velocity;
		const float flatDeltaX = down_o[0] - start_o[0];
		const float flatDeltaY = down_o[1] - start_o[1];

		if (iBumps || (pml->groundPlane && pml->groundTrace.normal[2] < 0.8999999761581421f))
		{
			vec3 up = start_o;
			up[2] += fStepSize + 1.0f;

			PM_PlayerTrace(pm, &trace, start_o, pm->mins, pm->maxs, up, ps->clientNum, pm->tracemask);
			fStepAmount = (fStepSize + 1.0f) * trace.fraction - 1.0f;

			if (fStepAmount >= 1.0f)
			{
				ps->origin = vec3(up[0], up[1], fStepAmount + start_o[2]);
				ps->velocity = start_v;
				PM_SlideMove(pm, pml, gravity);
			}
			else
			{
				fStepAmount = 0.0f;
			}
		}
		if (bHadGround || fStepAmount != 0.0f)
		{
			vec3 down = ps->origin;
			down[2] -= fStepAmount;
			if (bHadGround)
				down[2] -= 9.0f;

			PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, down, ps->clientNum, pm->tracemask);

			// Stepping onto another player is refused outright.
			if (Trace_GetEntityHitId(trace) < 64)
			{
				ps->origin = down_o;
				ps->velocity = down_v;
				return;
			}
			if (trace.fraction >= 1.0f)
			{
				if (fStepAmount != 0.0f)
					ps->origin[2] -= fStepAmount;
			}
			else
			{
				if (!trace.walkable && trace.normal[2] < 0.300000011920929f)
				{
					ps->origin = down_o;
					ps->velocity = down_v;
					return;
				}
				ps->origin = Vec3Lerp(ps->origin, down, trace.fraction);
				PM_ProjectVelocity(ps->velocity, trace.normal, ps->velocity);
			}
		}
		const float stepDeltaX = ps->origin[0] - start_o[0];
		const float stepDeltaY = ps->origin[1] - start_o[1];
		const float stepped = stepDeltaX * start_v[0] + stepDeltaY * start_v[1];
		const float flat = start_v[1] * flatDeltaY + start_v[0] * flatDeltaX;

		// The step only stands if it got the player further along their original velocity.
		if (stepped <= flat + EQUAL_EPSILON || (jumping && Jump_IsPlayerAboveMax(ps)))
		{
			ps->origin = down_o;
			ps->velocity = down_v;
			fStepAmount = 0.0f;

			if (bHadGround)
			{
				vec3 down = ps->origin;
				down[2] -= 9.0f;

				PM_PlayerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, down, ps->clientNum, pm->tracemask);
				if (trace.fraction < 1.0f)
				{
					const vec3 endpos = Vec3Lerp(ps->origin, down, trace.fraction);
					fStepAmount = endpos[2] - ps->origin[2];
					ps->origin = endpos;
					PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity, PM_OVERCLIP);
				}
			}
		}
		if (jumping)
			Jump_ClampVelocity(ps, down_o);

		if (bHadGround && ps->pm_type < PM_DEAD && PM_VerifyPronePosition(pm, start_o, start_v))
		{
			if (std::fabs(ps->origin[2] - down_o[2]) > 0.5f)
			{
				const int iDelta = static_cast<int>(ps->origin[2] - down_o[2]);
				if (iDelta)
				{
					if (pm->viewChangeTime < ps->commandTime)
					{
						pm->viewChange += ps->origin[2] - down_o[2];
						pm->viewChangeTime = ps->commandTime;
					}
					// Climbing a step costs speed in proportion to how much of it was climbed.
					const float climbed = std::fabs(ps->origin[2] - start_o[2]);
					const float fSpeedScale = 1.0f - 0.80000001f + (1.0f - climbed / fStepSize) * 0.80000001f;

					ps->velocity *= fSpeedScale;
					pm->xyspeed = Vec2Length(ps->velocity);

					// The bobCycle/footstep event this would raise is not ported.
				}
			}
		}
	}

	void PM_AirMove(pmove_t* pm, pml_t* pml)
	{
		playerState_s* ps = pm->ps;

		PM_Friction(ps, pml);

		const float fmove = static_cast<float>(pm->cmd.forwardmove);
		const float smove = static_cast<float>(pm->cmd.rightmove);
		const float scale = PM_CmdScale(ps, &pm->cmd);

		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;
		Vec3Normalize(pml->forward);
		Vec3Normalize(pml->right);

		vec3 wishvel(0.0f);
		for (int i = 0; i < 2; ++i)
			wishvel[i] = pml->forward[i] * fmove + pml->right[i] * smove;

		vec3 wishdir(wishvel[0], wishvel[1], 0.0f);
		const float wishspeed = Vec3Normalize(wishdir) * scale;

		PM_Accelerate(ps, pml, wishdir, wishspeed, 1.0f);

		if (pml->groundPlane)
			PM_ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity, PM_OVERCLIP);

		PM_StepSlideMove(pm, pml, true);
		PM_SetMovementDir(pm, pml);
	}

	void PM_WalkMove(pmove_t* pm, pml_t* pml)
	{
		playerState_s* ps = pm->ps;

		if (ps->pm_flags & PMF_JUMPING)
			Jump_ApplySlowdown(ps);

		if (ps->pm_flags & PMF_SPRINTING)
		{
			pm->cmd.rightmove = static_cast<char>(static_cast<int>(pm->cmd.rightmove
				* DvarFloat("player_sprintStrafeSpeedScale", 0.667f)));
		}
		if (Jump_Check(pm, pml))
		{
			PM_AirMove(pm, pml);
			return;
		}
		PM_Friction(ps, pml);

		const float fmove = static_cast<float>(pm->cmd.forwardmove);
		const float smove = static_cast<float>(pm->cmd.rightmove);
		const float scale = PM_CmdScale_Walk(pm, &pm->cmd);

		// PM_DamageScale_Walk and the damageTimer decay are part of the flinch system, not ported.

		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;
		Vec2Normalize(pml->forward);
		Vec2Normalize(pml->right);

		const vec3 wishvel(fmove * pml->forward[0] + smove * pml->right[0],
			fmove * pml->forward[1] + smove * pml->right[1], 0.0f);

		vec3 wishdir;
		const float wishspeed = Vec3NormalizeTo(wishvel, wishdir) * scale;

		PM_ProjectVelocity(wishdir, pml->groundTrace.normal, wishdir);

		const int iStance = PM_GetEffectiveStance(ps);
		float acceleration;

		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || (ps->pm_flags & PMF_TIME_KNOCKBACK))
			acceleration = 1.0f;
		else if (iStance == 1)
			acceleration = 19.0f;
		else if (iStance == 2)
			acceleration = 12.0f;
		else
			acceleration = 9.0f;

		if (ps->pm_flags & PMF_TIME_HARDLANDING)
			acceleration *= 0.25f;

		PM_Accelerate(ps, pml, wishdir, wishspeed, acceleration);

		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || (ps->pm_flags & PMF_TIME_KNOCKBACK))
			ps->velocity[2] -= static_cast<float>(ps->gravity) * pml->frametime;

		PM_ProjectVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity);

		if (ps->velocity[0] != 0.0f || ps->velocity[1] != 0.0f)
			PM_StepSlideMove(pm, pml, false);

		PM_SetMovementDir(pm, pml);
	}

	void PM_GroundTrace(pmove_t* pm, pml_t* pml)
	{
		playerState_s* ps = pm->ps;
		trace_t trace = {};

		vec3 start(ps->origin[0], ps->origin[1], 0.0f);
		vec3 point(ps->origin[0], ps->origin[1], 0.0f);

		if (ps->eFlags & EF_TURRET_ACTIVE)
		{
			start[2] = ps->origin[2];
			point[2] = ps->origin[2] - 1.0f;
		}
		else
		{
			start[2] = ps->origin[2] + 0.25f;
			point[2] = ps->origin[2] - 0.25f;
		}
		PM_PlayerTrace(pm, &trace, start, pm->mins, pm->maxs, point, ps->clientNum, pm->tracemask);
		pml->groundTrace = trace;

		if (trace.allsolid && !PM_CorrectAllSolid(pm, pml, &trace))
			return;

		if (trace.startsolid)
		{
			start[2] = ps->origin[2] - EQUAL_EPSILON;
			PM_PlayerTrace(pm, &trace, start, pm->mins, pm->maxs, point, ps->clientNum, pm->tracemask);

			if (trace.startsolid)
			{
				ps->groundEntityNum = ENTITYNUM_NONE;
				pml->groundPlane = 0;
				pml->almostGroundPlane = 0;
				pml->walking = 0;
				return;
			}
			pml->groundTrace = trace;
		}
		if (trace.fraction == 1.0f)
		{
			PM_GroundTraceMissed(pm, pml);
			return;
		}
		// Thrown off the ground: only a ladder or a downward/glancing velocity keeps contact.
		if (!(ps->pm_flags & PMF_LADDER) && ps->velocity[2] > 0.0f
			&& glm::dot(ps->velocity, trace.normal) > 10.0f)
		{
			pml->almostGroundPlane = 0;
			ps->groundEntityNum = ENTITYNUM_NONE;
			pml->groundPlane = 0;
			pml->walking = 0;
			return;
		}
		if (trace.walkable)
		{
			pml->groundPlane = 1;
			pml->almostGroundPlane = 1;
			pml->walking = 1;

			if (ps->groundEntityNum == ENTITYNUM_NONE)
				PM_CrashLand(ps, pml);

			ps->groundEntityNum = Trace_GetEntityHitId(trace);
			PM_AddTouchEnt(pm, ps->groundEntityNum);
		}
		else
		{
			ps->groundEntityNum = ENTITYNUM_NONE;
			pml->groundPlane = 1;
			pml->almostGroundPlane = 1;
			pml->walking = 0;
			Jump_ClearState(ps);
		}
	}

	void Jump_ClearState(playerState_s* ps)
	{
		ps->pm_flags &= ~PMF_JUMPING;
		ps->jumpOriginZ = 0.0f;
	}

	bool Jump_Check(pmove_t* pm, pml_t* pml)
	{
		playerState_s* ps = pm->ps;

		if (ps->pm_flags & PMF_NO_JUMP)
			return false;
		// jumpTime is the serverTime of the last jump, not a countdown: no second jump for 500 ms.
		if (pm->cmd.serverTime - ps->jumpTime < 500)
			return false;
		if (ps->pm_flags & PMF_RESPAWNED)
			return false;
		if (ps->pm_flags & PMF_MANTLE)
			return false;
		if (ps->pm_type >= PM_DEAD)
			return false;
		if (PM_GetEffectiveStance(ps))
			return false;
		if (!(pm->cmd.buttons & BUTTON_JUMP))
			return false;

		// A jump fires on the press edge only. A held button strips its own bit out of pm->cmd,
		// and since Pmove copies cmd into oldcmd afterwards the strip carries into the next step.
		if (pm->oldcmd.buttons & BUTTON_JUMP)
		{
			pm->cmd.buttons &= ~BUTTON_JUMP;
			return false;
		}
		Jump_Start(pm, pml, DvarFloat("jump_height", 39.0f));

		if (ps->pm_flags & PMF_LADDER)
			Jump_PushOffLadder(ps, pml);

		return true;
	}

	void PmoveSingle(pmove_t* pm)
	{
		playerState_s* ps = pm->ps;

		// Animation conditions, weapon state and torso anims are not ported.

		if (ps->pm_flags & PMF_MELEE_CHARGE)
			pm->cmd.forwardmove = 127;

		if (ps->pm_flags & PMF_FROZEN)
		{
			pm->cmd.buttons &= 0x1300;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
			ps->velocity = vec3(0.0f);
		}
		else if (ps->pm_flags & PMF_RESPAWNED)
		{
			pm->cmd.buttons &= 0x1301;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
			ps->velocity = vec3(0.0f);
		}
		else if (PM_IsPlayerFrozenByWeapon(ps))
		{
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
			pm->cmd.buttons &= ~0x4C0;
			ps->velocity = vec3(0.0f);
		}
		if (pm->cmd.buttons & CMD_BUTTON_NO_INPUT)
		{
			pm->cmd.buttons &= 0x101B02;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
		}
		ps->pm_flags &= ~PMF_NO_PRONE;

		if (ps->pm_type >= PM_DEAD)
			pm->tracemask &= ~CONTENTS_BODY;

		if (!(ps->pm_flags & PMF_PRONE_BIT) || BG_UsingSniperScope(ps))
		{
			ps->pm_flags &= ~PMF_PRONEMOVE_OVERRIDDEN;
		}
		else
		{
			const bool pushedForward = pm->cmd.forwardmove != pm->oldcmd.forwardmove
				&& std::abs(static_cast<int>(pm->oldcmd.forwardmove)) < std::abs(static_cast<int>(pm->cmd.forwardmove));
			const bool pushedRight = pm->cmd.rightmove != pm->oldcmd.rightmove
				&& std::abs(static_cast<int>(pm->oldcmd.rightmove)) < std::abs(static_cast<int>(pm->cmd.rightmove));

			if (!pushedForward && !pushedRight
				&& !(ps->pm_flags & PMF_SIGHT_AIMING)
				&& (ps->weaponstate <= 4 || ps->weaponstate == 7))
			{
				ps->pm_flags &= ~PMF_PRONEMOVE_OVERRIDDEN;
			}
		}
		const int stance = PM_GetEffectiveStance(ps);

		if ((ps->pm_flags & PMF_SIGHT_AIMING) && stance == 1 && !BG_UsingSniperScope(ps))
		{
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
		}
		if (pm->cmd.buttons & CMD_BUTTON_NO_INPUT)
			ps->eFlags |= EF_NO_INPUT;
		else
			ps->eFlags &= ~EF_NO_INPUT;

		ps->eFlags &= ~EF_FIRING;
		if (ps->pm_type != PM_INTERMISSION
			&& !(ps->pm_flags & PMF_RESPAWNED)
			&& (!ps->weaponstate || ps->weaponstate == 5)
			&& (pm->cmd.buttons & BUTTON_ATTACK))
		{
			ps->eFlags |= EF_FIRING;
		}
		// The only place bit 10 is ever cleared: releasing attack and use ends respawn protection,
		// and with it the block Jump_Check reads through PMF_RESPAWNED.
		if (ps->pm_type < PM_DEAD && !(pm->cmd.buttons & 0x101))
			ps->pm_flags &= ~PMF_RESPAWNED;

		pml_t pml = {};

		pml.msec = pm->cmd.serverTime - ps->commandTime;
		if (pml.msec < 1)
			pml.msec = 1;
		else if (pml.msec > 200)
			pml.msec = 200;

		ps->commandTime = pm->cmd.serverTime;
		pml.previous_origin = ps->origin;
		pml.previous_velocity = ps->velocity;
		pml.frametime = pml.msec * EQUAL_EPSILON;

		// PM_AdjustAimSpreadScale is weapon spread only.

		PM_UpdateViewAngles(ps, static_cast<float>(pml.msec), &pm->cmd, pm->handler);
		AngleVectors(ps->viewangles, &pml.forward, &pml.right, &pml.up);

		if (pm->cmd.forwardmove < 0)
			ps->pm_flags |= PMF_BACKWARDS_RUN;
		else if (pm->cmd.forwardmove > 0 || pm->cmd.rightmove)
			ps->pm_flags &= ~PMF_BACKWARDS_RUN;

		if (ps->pm_type >= PM_LASTSTAND)
		{
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
		}
		if (stance == 1 && (ps->pm_flags & PMF_PRONEMOVE_OVERRIDDEN))
		{
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
		}
		PM_MeleeChargeUpdate(pm, &pml);

		switch (ps->pm_type)
		{
		case PM_NORMAL_LINKED:
		case PM_DEAD_LINKED:
			PM_ClearLadderFlag(ps);
			ps->groundEntityNum = ENTITYNUM_NONE;
			pml.walking = 0;
			pml.groundPlane = 0;
			pml.almostGroundPlane = 0;
			ps->velocity = vec3(0.0f);
			PM_UpdateAimDownSightFlag(pm, &pml);
			PM_UpdateSprint(pm, &pml);
			PM_UpdatePlayerWalkingFlag(pm);
			PM_DropTimers(ps, &pml);
			PM_CheckDuck(pm, &pml);
			return;

		case PM_NOCLIP:
			PM_ClearLadderFlag(ps);
			PM_UpdateAimDownSightFlag(pm, &pml);
			PM_UpdateSprint(pm, &pml);
			PM_UpdatePlayerWalkingFlag(pm);
			PM_DropTimers(ps, &pml);
			PM_NoclipMove(pm, &pml);
			return;

		// PM_UFOMove is not ported; the noclip move stands in for it.
		case PM_UFO:
			PM_ClearLadderFlag(ps);
			PM_UpdateAimDownSightFlag(pm, &pml);
			PM_UpdateSprint(pm, &pml);
			PM_UpdatePlayerWalkingFlag(pm);
			PM_DropTimers(ps, &pml);
			PM_NoclipMove(pm, &pml);
			return;

		case PM_SPECTATOR:
			PM_ClearLadderFlag(ps);
			PM_UpdateAimDownSightFlag(pm, &pml);
			PM_UpdateSprint(pm, &pml);
			PM_UpdatePlayerWalkingFlag(pm);
			PM_CheckDuck(pm, &pml);
			PM_DropTimers(ps, &pml);
			PM_FlyMove(pm, &pml);
			return;

		case PM_INTERMISSION:
			PM_ClearLadderFlag(ps);
			PM_UpdateAimDownSightFlag(pm, &pml);
			PM_UpdateSprint(pm, &pml);
			return;

		case PM_LASTSTAND:
			PM_ClearLadderFlag(ps);
			ps->eFlags &= ~EF_TURRET_ACTIVE;
			break;

		default:
			break;
		}

		if (ps->eFlags & EF_TURRET_ACTIVE)
		{
			PM_ClearLadderFlag(ps);
			ps->groundEntityNum = ENTITYNUM_NONE;
			pml.walking = 0;
			pml.groundPlane = 0;
			pml.almostGroundPlane = 0;
			ps->velocity = vec3(0.0f);
			PM_UpdateAimDownSightFlag(pm, &pml);
			PM_UpdateSprint(pm, &pml);
			PM_UpdatePlayerWalkingFlag(pm);
			PM_DropTimers(ps, &pml);
			PM_CheckDuck(pm, &pml);
			return;
		}
		// Mantling is not ported, so the mantle branch never runs.
		PM_UpdateAimDownSightFlag(pm, &pml);
		PM_UpdateSprint(pm, &pml);
		PM_UpdatePlayerWalkingFlag(pm);
		PM_CheckDuck(pm, &pml);
		GroundTrace(pm, &pml);

		PM_DropTimers(ps, &pml);

		if (ps->pm_type >= PM_LASTSTAND)
			PM_DeadMove(ps, &pml);

		// Ladders are not ported: PM_CheckLadderMove never sets PMF_LADDER here.
		if (pml.walking)
			WalkMove(pm, &pml);
		else
			AirMove(pm, &pml);

		GroundTrace(pm, &pml);

		// PM_Footsteps, PM_Weapon and PM_FoliageSounds are not ported.

		// If the move covered far less ground than the velocity claims, the velocity is rewritten
		// from the distance actually travelled.
		const vec3 move = ps->origin - pml.previous_origin;
		const float realVelSqrd = glm::dot(move, move) / (pml.frametime * pml.frametime);
		const float supposedVelSqrd = glm::dot(ps->velocity, ps->velocity);

		if (realVelSqrd < supposedVelSqrd * 0.25f)
			ps->velocity = move * (1.0f / pml.frametime);

		// oldVelocity is a one pole filter over the horizontal velocity whose coefficient is the
		// step's own frametime, and it is what PM_PlayerInertia reads.
		vec2 velocityChange(ps->velocity[0] - ps->oldVelocity[0], ps->velocity[1] - ps->oldVelocity[1]);
		const float lerp = (1.0f - pml.frametime) < 0.0f ? 1.0f : pml.frametime;

		ps->oldVelocity[0] += lerp * velocityChange[0];
		ps->oldVelocity[1] += lerp * velocityChange[1];

		Sys_SnapVector(ps->velocity);
	}

	void Pmove(pmove_t* pm)
	{
		playerState_s* ps = pm->ps;
		const int finalTime = pm->cmd.serverTime;

		if (finalTime < ps->commandTime)
			return;

		if (finalTime > ps->commandTime + 1000)
			ps->commandTime = finalTime - 1000;

		pm->numtouch = 0;

		while (ps->commandTime != finalTime)
		{
			int msec = finalTime - ps->commandTime;
			if (msec > 66)
				msec = 66;

			pm->cmd.serverTime = msec + ps->commandTime;
			PmoveSingle(pm);
			pm->oldcmd = pm->cmd;
		}
	}

	pmove_t MakePmove(playerState_s* ps, const usercmd_s& cmd, const usercmd_s& oldcmd)
	{
		pmove_t pm = {};

		pm.ps = ps;
		pm.cmd = cmd;
		pm.oldcmd = oldcmd;
		pm.mins = { -15.0f, -15.0f, 0.0f };
		pm.maxs = { 15.0f, 15.0f, 70.0f };

		if (ps->viewHeightCurrent == 11.0f)
			pm.maxs[2] = 30.0f;
		else if (ps->viewHeightCurrent == 40.0f)
			pm.maxs[2] = 50.0f;

		pm.tracemask = ps->pm_type < PM_DEAD ? MASK_PLAYERSOLID : (MASK_PLAYERSOLID & ~CONTENTS_BODY);
		if (ps->pm_type == PM_SPECTATOR)
			pm.tracemask &= ~0x02010000;

		// CG_PredictPlayerState runs the client trace handler; the simulator has only the one.
		pm.handler = 0;
		pm.viewChange = 0.0f;
		pm.viewChangeTime = 0;
		return pm;
	}
}
