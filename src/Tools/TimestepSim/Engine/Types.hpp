#pragma once
// Engine types the movement code needs, lifted from src/Game/Definitions/Structs.hpp with the
// field names kept identical so the shipping movement sources compile against this unchanged.
// Binary layout is deliberately not preserved: nothing here is ever mapped onto game memory.
#include <cmath>
#include <cstdint>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;

#define API
#define BIT(n) (1U << (n))

#define MASK_PLAYERSOLID 0x02810011
#define ENTITYNUM_NONE 1023

namespace IW3SR
{
	enum pmtype_t
	{
		PM_NORMAL = 0x0,
		PM_NORMAL_LINKED = 0x1,
		PM_NOCLIP = 0x2,
		PM_UFO = 0x3,
		PM_SPECTATOR = 0x4,
		PM_INTERMISSION = 0x5,
		PM_LASTSTAND = 0x6,
		PM_DEAD = 0x7,
		PM_DEAD_LINKED = 0x8,
	};

	enum PMoveFlags
	{
		PMF_NONE = 0,
		PMF_DUCKED = BIT(1),
		PMF_MANTLE = BIT(2),
		PMF_LADDER = BIT(3),
		PMF_SIGHT_AIMING = BIT(4),
		PMF_BACKWARDS_RUN = BIT(5),
		PMF_LADDER_DOWN = BIT(5) | BIT(3),
		PMF_WALKING = BIT(6),
		PMF_TIME_HARDLANDING = BIT(7),
		PMF_TIME_KNOCKBACK = BIT(8),
		PMF_PRONEMOVE_OVERRIDDEN = BIT(9),
		PMF_RESPAWNED = BIT(10),
		PMF_FROZEN = BIT(11),
		PMF_NO_PRONE = BIT(12),
		PMF_LADDER_FALL = BIT(13),
		PMF_JUMPING = BIT(14),
		// Deliberately the mod's value, not the engine's. bg_local.h has prone as bit 0 alone;
		// Structs.hpp aliases it onto PMF_JUMPING, and the movement sources compiled in here read
		// that, so the simulator has to as well. Measured inert: WalkMove never runs with
		// PMF_JUMPING set. Engine/Pmove.cpp binds the engine's bit for the engine half.
		PMF_PRONE = BIT(14) | BIT(0),
		PMF_SPRINTING = BIT(15),
		PMF_SHELLSHOCKED = BIT(16),
		PMF_MELEE_CHARGE = BIT(17),
		PMF_NO_SPRINT = BIT(18),
		PMF_NO_JUMP = BIT(19),
		PMF_VEHICLE_ATTACHED = BIT(20)
	};

	// Only the surface bit the movement code branches on.
	enum SurfaceFlags
	{
		SURF_SLICK = 0x2,
	};

	enum TraceHitType
	{
		TRACE_HITTYPE_NONE = 0x0,
		TRACE_HITTYPE_ENTITY = 0x1,
		TRACE_HITTYPE_DYNENT_MODEL = 0x2,
		TRACE_HITTYPE_DYNENT_BRUSH = 0x3,
	};

	struct SprintState
	{
		int sprintButtonUpRequired;
		int sprintDelay;
		int lastSprintStart;
		int lastSprintEnd;
		int sprintStartMaxLength;
	};

	struct MantleState
	{
		float yaw;
		int timer;
		int transIndex;
		int flags;
	};

	struct playerState_s
	{
		int commandTime;
		int pm_type;
		int bobCycle;
		int pm_flags;
		int weapFlags;
		int otherFlags;
		int pm_time;
		vec3 origin;
		vec3 velocity;
		vec2 oldVelocity;
		int weaponTime;
		int weaponDelay;
		int grenadeTimeLeft;
		int throwBackGrenadeOwner;
		int throwBackGrenadeTimeLeft;
		int weaponRestrictKickTime;
		int foliageSoundTime;
		int gravity;
		float leanf;
		int speed;
		vec3 delta_angles;
		int groundEntityNum;
		vec3 vLadderVec;
		int jumpTime;
		float jumpOriginZ;
		int legsTimer;
		int legsAnim;
		int torsoTimer;
		int torsoAnim;
		int legsAnimDuration;
		int torsoAnimDuration;
		int damageTimer;
		int damageDuration;
		int flinchYawAnim;
		int movementDir;
		int eFlags;
		int eventSequence;
		int events[4];
		uint32_t eventParms[4];
		int oldEventSequence;
		int clientNum;
		int offHandIndex;
		int offhandSecondary;
		uint32_t weapon;
		int weaponstate;
		uint32_t weaponShotCount;
		float fWeaponPosFrac;
		int adsDelayTime;
		int spreadOverride;
		int spreadOverrideState;
		int viewmodelIndex;
		vec3 viewangles;
		int viewHeightTarget;
		float viewHeightCurrent;
		int viewHeightLerpTime;
		int viewHeightLerpTarget;
		int viewHeightLerpDown;
		vec2 viewAngleClampBase;
		vec2 viewAngleClampRange;
		int damageEvent;
		int damageYaw;
		int damagePitch;
		int damageCount;
		int stats[5];
		int ammo[128];
		int ammoclip[128];
		uint32_t weapons[4];
		uint32_t weaponold[4];
		uint32_t weaponrechamber[4];
		float proneDirection;
		float proneDirectionPitch;
		float proneTorsoPitch;
		int viewlocked;
		int viewlocked_entNum;
		int cursorHint;
		int cursorHintString;
		int cursorHintEntIndex;
		int iCompassPlayerInfo;
		int radarEnabled;
		int locationSelectionInfo;
		SprintState sprintState;
		float fTorsoPitch;
		float fWaistPitch;
		float holdBreathScale;
		int holdBreathTimer;
		float moveSpeedScaleMultiplier;
		MantleState mantleState;
		float meleeChargeYaw;
		int meleeChargeDist;
		int meleeChargeTime;
		int perks;
		int entityEventSequence;
		int weapAnim;
		float aimSpreadScale;
		int shellshockIndex;
		int shellshockTime;
		int shellshockDuration;
		int deltaTime;
		int killCamEntity;
	};

	struct usercmd_s
	{
		int serverTime;
		int buttons;
		int angles[3];
		char weapon;
		char offHandIndex;
		char forwardmove;
		char rightmove;
		float meleeChargeYaw;
		char meleeChargeDist;
		char selectedLocation[2];
	};

	struct alignas(4) trace_t
	{
		float fraction;
		vec3 normal;
		int surfaceFlags;
		int contents;
		const char* material;
		TraceHitType hitType;
		uint16_t hitId;
		uint16_t modelIndex;
		uint16_t partName;
		uint16_t partGroup;
		bool allsolid;
		bool startsolid;
		bool walkable;
	};

	struct pml_t
	{
		vec3 forward;
		vec3 right;
		vec3 up;
		float frametime;
		int msec;
		int walking;
		int groundPlane;
		int almostGroundPlane;
		trace_t groundTrace;
		float impactSpeed;
		vec3 previous_origin;
		vec3 previous_velocity;
	};

	struct pmove_t
	{
		playerState_s* ps;
		usercmd_s cmd;
		usercmd_s oldcmd;
		int tracemask;
		int numtouch;
		int touchents[32];
		vec3 mins;
		vec3 maxs;
		float xyspeed;
		int proneChange;
		float maxSprintTimeMultiplier;
		bool mantleStarted;
		vec3 mantleEndPos;
		int mantleDuration;
		int viewChangeTime;
		float viewChange;
		char handler;
	};

	// Buttons the movement code reads. BUTTON_JUMP shares bit 10 with PMF_RESPAWNED, which is a
	// coincidence of the engine layout and not a relationship.
	enum UserCmdButtons
	{
		BUTTON_ATTACK = 0x1,
		BUTTON_SPRINT = 0x2,
		BUTTON_PRONE = 0x100,
		BUTTON_CROUCH = 0x200,
		BUTTON_JUMP = 0x400,
		BUTTON_ADS = 0x800,
		BUTTON_TEMP_STANCE = 0x1000,
	};

	constexpr float SHORT2ANGLE_SCALE = 360.0f / 65536.0f;
	constexpr float ANGLE2SHORT_SCALE = 65536.0f / 360.0f;

	inline float SHORT2ANGLE(int x)
	{
		return static_cast<float>(static_cast<int16_t>(x)) * SHORT2ANGLE_SCALE;
	}
	inline int ANGLE2SHORT(float x)
	{
		return static_cast<int16_t>(static_cast<int>(x * ANGLE2SHORT_SCALE) & 0xFFFF);
	}
}
