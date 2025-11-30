#pragma once
#include "DX9/Base.hpp"

#pragma warning(push)
#pragma warning(disable : 4359)

namespace IW3SR
{
	struct XAnimTree_s;
	struct FxEffectDef;
	struct gentity_s;
	struct GfxCell;
	struct GfxPortal;
	struct GfxViewInfo;
	struct itemDef_s;
	struct pathnode_t;
	struct pathnode_tree_t;
	struct scr_vehicle_s;
	struct snd_alias_list_t;

	typedef void (*xfunction_t)();

	struct scr_function_s
	{
		struct scr_function_s* next;
		char* name;
		xfunction_t function;
		bool developer;
	};

	enum hitLocation_t
	{
		HITLOC_NONE = 0x0,
		HITLOC_HELMET = 0x1,
		HITLOC_HEAD = 0x2,
		HITLOC_NECK = 0x3,
		HITLOC_TORSO_UPR = 0x4,
		HITLOC_TORSO_LWR = 0x5,
		HITLOC_R_ARM_UPR = 0x6,
		HITLOC_L_ARM_UPR = 0x7,
		HITLOC_R_ARM_LWR = 0x8,
		HITLOC_L_ARM_LWR = 0x9,
		HITLOC_R_HAND = 0xA,
		HITLOC_L_HAND = 0xB,
		HITLOC_R_LEG_UPR = 0xC,
		HITLOC_L_LEG_UPR = 0xD,
		HITLOC_R_LEG_LWR = 0xE,
		HITLOC_L_LEG_LWR = 0xF,
		HITLOC_R_FOOT = 0x10,
		HITLOC_L_FOOT = 0x11,
		HITLOC_GUN = 0x12,
		HITLOC_NUM = 0x13,
	};

	enum scriptAnimEventTypes_t
	{
		ANIM_ET_PAIN = 0x0,
		ANIM_ET_DEATH = 0x1,
		ANIM_ET_FIREWEAPON = 0x2,
		ANIM_ET_JUMP = 0x3,
		ANIM_ET_JUMPBK = 0x4,
		ANIM_ET_LAND = 0x5,
		ANIM_ET_DROPWEAPON = 0x6,
		ANIM_ET_RAISEWEAPON = 0x7,
		ANIM_ET_CLIMB_MOUNT = 0x8,
		ANIM_ET_CLIMB_DISMOUNT = 0x9,
		ANIM_ET_RELOAD = 0xA,
		ANIM_ET_CROUCH_TO_PRONE = 0xB,
		ANIM_ET_PRONE_TO_CROUCH = 0xC,
		ANIM_ET_STAND_TO_CROUCH = 0xD,
		ANIM_ET_CROUCH_TO_STAND = 0xE,
		ANIM_ET_STAND_TO_PRONE = 0xF,
		ANIM_ET_PRONE_TO_STAND = 0x10,
		ANIM_ET_MELEEATTACK = 0x11,
		ANIM_ET_KNIFE_MELEE = 0x12,
		ANIM_ET_KNIFE_MELEE_CHARGE = 0x13,
		ANIM_ET_SHELLSHOCK = 0x14,
		NUM_ANIM_EVENTTYPES = 0x15,
	};

	struct ScreenPlacement
	{
		vec2 scaleVirtualToReal;
		vec2 scaleVirtualToFull;
		vec2 scaleRealToVirtual;
		vec2 virtualViewableMin;
		vec2 virtualViewableMax;
		vec2 realViewportSize;
		vec2 realViewableMin;
		vec2 realViewableMax;
		float subScreenLeft;
	};

	struct Message
	{
		int startTime;
		int endTime;
	};

	struct MessageLine
	{
		int messageIndex;
		int textBufPos;
		int textBufSize;
		int typingStartTime;
		int lastTypingSoundTime;
		int flags;
	};

	struct MessageWindow
	{
		MessageLine* lines;
		Message* messages;
		char* circularTextBuffer;
		int textBufSize;
		int lineCount;
		int padding;
		int scrollTime;
		int fadeIn;
		int fadeOut;
		int textBufPos;
		int firstLineIndex;
		int activeLineCount;
		int messageIndex;
	};

	struct MessageBuffer
	{
		char gamemsgText[4][2048];
		MessageWindow gamemsgWindows[4];
		MessageLine gamemsgLines[4][12];
		Message gamemsgMessages[4][12];
		char miniconText[4096];
		MessageWindow miniconWindow;
		MessageLine miniconLines[100];
		Message miniconMessages[100];
		char errorText[1024];
		MessageWindow errorWindow;
		MessageLine errorLines[5];
		Message errorMessages[5];
	};

	struct field_t
	{
		int cursor;
		int scroll;
		int drawWidth;
		int widthInPixels;
		float charHeight;
		int fixedSize;
		char buffer[256];
	};

	struct Con
	{
		int initialized;
		MessageWindow consoleWindow;
		MessageLine consoleLines[1024];
		Message consoleMessages[1024];
		char consoleText[32768];
		char textTempLine[512];
		uint32_t lineOffset;
		int displayLineOffset;
		int prevChannel;
		bool outputVisible;
		int fontHeight;
		int visibleLineCount;
		int visiblePixelWidth;
		float screenMin[2];
		float screenMax[2];
		MessageBuffer messageBuffer[1];
		float color[4];
	};

	enum ConChannel
	{
		CON_CHANNEL_LOG = 0x0,
		CON_CHANNEL_ERROR = 0x1,
		CON_CHANNEL_GAMENOTIFY = 0x2,
		CON_CHANNEL_BOLDGAME = 0x3,
		CON_CHANNEL_SUBTITLE = 0x4,
		CON_CHANNEL_OBITUARY = 0x5,
		CON_CHANNEL_LOGFILEONLY = 0x6,
		CON_CHANNEL_CONSOLEONLY = 0x7,
		CON_CHANNEL_GFX = 0x8,
		CON_CHANNEL_SOUND = 0x9,
		CON_CHANNEL_FILES = 0xA,
		CON_CHANNEL_DEVGUI = 0xB,
		CON_CHANNEL_PROFILE = 0xC,
		CON_CHANNEL_UI = 0xD,
		CON_CHANNEL_CLIENT = 0xE,
		CON_CHANNEL_SERVER = 0xF,
		CON_CHANNEL_SYSTEM = 0x10,
		CON_CHANNEL_PLAYERWEAP = 0x11,
		CON_CHANNEL_AI = 0x12,
		CON_CHANNEL_ANIM = 0x13,
		CON_CHANNEL_PHYS = 0x14,
		CON_CHANNEL_FX = 0x15,
		CON_CHANNEL_LEADERBOARDS = 0x16,
		CON_CHANNEL_LIVE = 0x17,
		CON_CHANNEL_PARSERSCRIPT = 0x18,
		CON_CHANNEL_SCRIPT = 0x19,
		CON_CHANNEL_SPAWNSYSTEM = 0x1A,
		CON_CHANNEL_COOPINFO = 0x1B,
		CON_CHANNEL_SERVERDEMO = 0x1C,
		CON_CHANNEL_DDL = 0x1D,
		CON_CHANNEL_NETWORK = 0x1E,
		CON_CHANNEL_SCHEDULER = 0x1F,
		CON_CHANNEL_FIRST_DEBUG = 0x1F,
		CON_CHANNEL_TASK = 0x20,
		CON_CHANNEL_SPU = 0x21,
		CON_CHANNEL_FILEDL = 0x22,
		CON_CHANNEL_CVAR = 0x23,
		CON_CHANNEL_PLUGINS = 0x24,
		CON_CHANNEL_STEAM = 0x25,
		CON_CHANNEL_BUILTIN_COUNT = 0x26,
		CON_CHANNEL_SR = 0x27
	};

	struct ConDrawInputGlob
	{
		char autoCompleteChoice[64];
		int matchIndex;
		int matchCount;
		const char* inputText;
		int inputTextLen;
		bool hasExactMatch;
		bool mayAutoComplete;
		float x;
		float y;
		float leftX;
		float fontHeight;
	};

	enum XAssetType
	{
		ASSET_TYPE_XMODELPIECES = 0x0,
		ASSET_TYPE_PHYSPRESET = 0x1,
		ASSET_TYPE_XANIMPARTS = 0x2,
		ASSET_TYPE_XMODEL = 0x3,
		ASSET_TYPE_MATERIAL = 0x4,
		ASSET_TYPE_TECHNIQUE_SET = 0x5,
		ASSET_TYPE_IMAGE = 0x6,
		ASSET_TYPE_SOUND = 0x7,
		ASSET_TYPE_SOUND_CURVE = 0x8,
		ASSET_TYPE_LOADED_SOUND = 0x9,
		ASSET_TYPE_CLIPMAP = 0xA,
		ASSET_TYPE_CLIPMAP_PVS = 0xB,
		ASSET_TYPE_COMWORLD = 0xC,
		ASSET_TYPE_GAMEWORLD_SP = 0xD,
		ASSET_TYPE_GAMEWORLD_MP = 0xE,
		ASSET_TYPE_MAP_ENTS = 0xF,
		ASSET_TYPE_GFXWORLD = 0x10,
		ASSET_TYPE_LIGHT_DEF = 0x11,
		ASSET_TYPE_UI_MAP = 0x12,
		ASSET_TYPE_FONT = 0x13,
		ASSET_TYPE_MENULIST = 0x14,
		ASSET_TYPE_MENU = 0x15,
		ASSET_TYPE_LOCALIZE_ENTRY = 0x16,
		ASSET_TYPE_WEAPON = 0x17,
		ASSET_TYPE_SNDDRIVER_GLOBALS = 0x18,
		ASSET_TYPE_FX = 0x19,
		ASSET_TYPE_IMPACT_FX = 0x1A,
		ASSET_TYPE_AITYPE = 0x1B,
		ASSET_TYPE_MPTYPE = 0x1C,
		ASSET_TYPE_CHARACTER = 0x1D,
		ASSET_TYPE_XMODELALIAS = 0x1E,
		ASSET_TYPE_RAWFILE = 0x1F,
		ASSET_TYPE_STRINGTABLE = 0x20,
		ASSET_TYPE_COUNT = 0x21,
		ASSET_TYPE_STRING = 0x21,
		ASSET_TYPE_ASSETLIST = 0x22,
	};

	enum ButtonFlags
	{
		BUTTON_FIRE = BIT(0),
		BUTTON_SPRINT = BIT(1),
		BUTTON_MELEE = BIT(2),
		BUTTON_USE = BIT(3),
		BUTTON_RELOAD = BIT(4),
		BUTTON_USE_RELOAD = BIT(5),
		BUTTON_LEAN_LEFT = BIT(6),
		BUTTON_LEAN_RIGHT = BIT(7),
		BUTTON_PRONE = BIT(8),
		BUTTON_CROUCH = BIT(9),
		BUTTON_JUMP = BIT(10),
		BUTTON_ADS = BIT(11),
		BUTTON_CROUCH_HOLD = BIT(12) | BIT(9),
		BUTTON_PRONE_HOLD = BIT(12) | BIT(8),
		BUTTON_STEADY_AIM = BIT(13),
		BUTTON_HOLD_BREATH = BIT(13) | BIT(1),
		BUTTON_FRAG = BIT(14),
		BUTTON_SPECIAL = BIT(15),
		BUTTON_NIGHTVISION = BIT(18),
		BUTTON_ADS_OFF = BIT(19),
		BUTTON_ADS_HOLD = BIT(19) | BIT(6),
		BUTTON_NO_INPUT = BIT(20)
	};

	enum weaponstate_t
	{
		WEAPON_READY = 0x0,
		WEAPON_RAISING = 0x1,
		WEAPON_RAISING_ALTSWITCH = 0x2,
		WEAPON_DROPPING = 0x3,
		WEAPON_DROPPING_QUICK = 0x4,
		WEAPON_FIRING = 0x5,
		WEAPON_RECHAMBERING = 0x6,
		WEAPON_RELOADING = 0x7,
		WEAPON_RELOADING_INTERUPT = 0x8,
		WEAPON_RELOAD_START = 0x9,
		WEAPON_RELOAD_START_INTERUPT = 0xA,
		WEAPON_RELOAD_END = 0xB,
		WEAPON_MELEE_INIT = 0xC,
		WEAPON_MELEE_FIRE = 0xD,
		WEAPON_MELEE_END = 0xE,
		WEAPON_OFFHAND_INIT = 0xF,
		WEAPON_OFFHAND_PREPARE = 0x10,
		WEAPON_OFFHAND_HOLD = 0x11,
		WEAPON_OFFHAND_START = 0x12,
		WEAPON_OFFHAND = 0x13,
		WEAPON_OFFHAND_END = 0x14,
		WEAPON_DETONATING = 0x15,
		WEAPON_SPRINT_RAISE = 0x16,
		WEAPON_SPRINT_LOOP = 0x17,
		WEAPON_SPRINT_DROP = 0x18,
		WEAPON_NIGHTVISION_WEAR = 0x19,
		WEAPON_NIGHTVISION_REMOVE = 0x1A,
		WEAPONSTATES_NUM = 0x1B,
	};

	struct cmd_function_s
	{
		cmd_function_s* next;
		const char* name;
		const char* args;
		const char* description;
		void(__cdecl* function)();
	};

	union DvarLimits
	{
		struct
		{
			int stringCount;
			const char** strings;
		} enumeration;
		struct
		{
			int min;
			int max;
		} integer;
		struct
		{
			float min;
			float max;
		} value;
		struct
		{
			float min;
			float max;
		} vector;
	};

	union DvarValue
	{
		bool enabled;
		int integer;
		uint32_t uinteger;
		float value;
		float vector[4];
		const char* string;
		char color[4];
	};

	enum class DvarType : int8_t
	{
		BOOLEAN = 0x0,
		VALUE = 0x1,
		VEC2 = 0x2,
		VEC3 = 0x3,
		VEC4 = 0x4,
		INTEGER = 0x5,
		ENUMERATION = 0x6,
		STRING = 0x7,
		COLOR = 0x8,
		RGB = 0x9
	};

	enum DvarFlags : uint16_t
	{
		DVAR_NONE = BIT(0),
		DVAR_SAVED = BIT(1),
		DVAR_USERINFO = BIT(2),	  // Sent to server on connect or change
		DVAR_SERVERINFO = BIT(3), // Sent in response to front end requests
		DVAR_REPLICATED = BIT(4),
		DVAR_WRITEPROTECTED = BIT(5),
		DVAR_LATCHED = BIT(6),
		DVAR_READONLY = BIT(7),
		DVAR_CHEATPROTECTED = BIT(8),
		DVAR_TEMP = BIT(9),
		DVAR_NORESTART = BIT(10),	// Do not clear when a cvar_restart is issued
		DVAR_USERCREATED = BIT(14), // Created by a set command
	};

	struct dvar_s
	{
		const char* name;
		const char* description;
		DvarFlags flags;
		DvarType type;
		bool modified;
		DvarValue current;
		DvarValue latched;
		DvarValue reset;
		DvarLimits domain;
		bool(__cdecl* domainFunc)(dvar_s*, DvarValue);
		dvar_s* hashNext;
	};

	using DB_XAssetSizeHandler_t = int (*)();

	enum DB_FILE_EXISTS_PATH
	{
		DB_PATH_ZONE = 0x0,
		DB_PATH_MAIN = 0x1,
		DB_PATH_USERMAPS = 0x2
	};

	enum XZONE_FLAGS
	{
		XZONE_LOC_POST_GFX = 0,
		XZONE_LOC_COMMON = BIT(0),
		XZONE_POST_GFX = BIT(1),
		XZONE_COMMON = BIT(2),
		XZONE_UI = BIT(3),
		XZONE_MAP = BIT(3),
		XZONE_MOD = BIT(4),
		XZONE_LOAD = BIT(5),
		XZONE_DEBUG = BIT(6)
	};

	struct XZoneInfo
	{
		const char* name;
		int allocFlags;
		int freeFlags;
	};

	struct SpawnVar
	{
		bool spawnVarsValid;
		int numSpawnVars;
		char* spawnVars[64][2];
		int numSpawnVarChars;
		char spawnVarChars[2048];
	};

	struct WinMouseVars_t
	{
		int oldButtonState;
		tagPOINT oldPos;
		bool mouseActive;
		bool mouseInitialized;
		char pad[2];
	};

	union PackedUnitVec
	{
		uint32_t packed;
		char array[4];
	};

	union GfxColor
	{
		uint32_t packed;
		char array[4];
	};

	struct GfxWorldVertex
	{
		float xyz[3];
		float binormalSign;
		GfxColor color;
		float texCoord[2];
		float lmapCoord[2];
		PackedUnitVec normal;
		PackedUnitVec tangent;
	};

	struct map_brushPlane_t
	{
		vec3 point;
	};

	struct map_brushSide_t
	{
		int brushSideIndex;
		map_brushPlane_t brushPlane[3];
	};

	struct map_patchQuads_t
	{
		int quadIndex;
		vec3 coords[4];
	};

	struct map_patchTris_t
	{
		int triIndex;
		GfxWorldVertex coords[3];
	};

	enum OffhandSecondaryClass
	{
		PLAYER_OFFHAND_SECONDARY_SMOKE = 0x0,
		PLAYER_OFFHAND_SECONDARY_FLASH = 0x1,
		PLAYER_OFFHAND_SECONDARIES_TOTAL = 0x2,
	};

	enum ViewLockTypes
	{
		PLAYERVIEWLOCK_NONE = 0x0,
		PLAYERVIEWLOCK_FULL = 0x1,
		PLAYERVIEWLOCK_WEAPONJITTER = 0x2,
		PLAYERVIEWLOCKCOUNT = 0x3,
	};

	enum ActionSlotType
	{
		ACTIONSLOTTYPE_DONOTHING = 0x0,
		ACTIONSLOTTYPE_SPECIFYWEAPON = 0x1,
		ACTIONSLOTTYPE_ALTWEAPONTOGGLE = 0x2,
		ACTIONSLOTTYPE_NIGHTVISION = 0x3,
		ACTIONSLOTTYPECOUNT = 0x4,
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

	struct ActionSlotParam_SpecifyWeapon
	{
		uint32_t index;
	};

	struct ActionSlotParam
	{
		ActionSlotParam_SpecifyWeapon specifyWeapon;
	};

	enum objectiveState_t
	{
		OBJST_EMPTY = 0x0,
		OBJST_ACTIVE = 0x1,
		OBJST_INVISIBLE = 0x2,
		OBJST_DONE = 0x3,
		OBJST_CURRENT = 0x4,
		OBJST_FAILED = 0x5,
		OBJST_NUMSTATES = 0x6,
	};

	struct objective_t
	{
		objectiveState_t state;
		vec3 origin;
		int entNum;
		int teamNum;
		int icon;
	};

	enum he_type_t
	{
		HE_TYPE_FREE = 0x0,
		HE_TYPE_TEXT = 0x1,
		HE_TYPE_VALUE = 0x2,
		HE_TYPE_PLAYERNAME = 0x3,
		HE_TYPE_MAPNAME = 0x4,
		HE_TYPE_GAMETYPE = 0x5,
		HE_TYPE_MATERIAL = 0x6,
		HE_TYPE_TIMER_DOWN = 0x7,
		HE_TYPE_TIMER_UP = 0x8,
		HE_TYPE_TENTHS_TIMER_DOWN = 0x9,
		HE_TYPE_TENTHS_TIMER_UP = 0xA,
		HE_TYPE_CLOCK_DOWN = 0xB,
		HE_TYPE_CLOCK_UP = 0xC,
		HE_TYPE_WAYPOINT = 0xD,
		HE_TYPE_COUNT = 0xE,
	};

	struct $C96EA5EC2ACBB9C0BF22693F316ACC67
	{
		char r;
		char g;
		char b;
		char a;
	};

	union hudelem_color_t
	{
		$C96EA5EC2ACBB9C0BF22693F316ACC67 __s0;
		int rgba;
	};

	struct hudelem_s
	{
		he_type_t type;
		float x;
		float y;
		float z;
		int targetEntNum;
		float fontScale;
		int font;
		int alignOrg;
		int alignScreen;
		hudelem_color_t color;
		hudelem_color_t fromColor;
		int fadeStartTime;
		int fadeTime;
		int label;
		int width;
		int height;
		int materialIndex;
		int offscreenMaterialIdx;
		int fromWidth;
		int fromHeight;
		int scaleStartTime;
		int scaleTime;
		float fromX;
		float fromY;
		int fromAlignOrg;
		int fromAlignScreen;
		int moveStartTime;
		int moveTime;
		int time;
		int duration;
		float value;
		int text;
		float sort;
		hudelem_color_t glowColor;
		int fxBirthTime;
		int fxLetterTime;
		int fxDecayStartTime;
		int fxDecayDuration;
		int soundID;
		int flags;
	};

	struct $15067B6A14D88D7E1E730369692C3A81
	{
		hudelem_s current[31];
		hudelem_s archival[31];
	};

	enum pmType_t
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
		OffhandSecondaryClass offhandSecondary;
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
		ViewLockTypes viewlocked;
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
		ActionSlotType actionSlotType[4];
		ActionSlotParam actionSlotParam[4];
		int entityEventSequence;
		int weapAnim;
		float aimSpreadScale;
		int shellshockIndex;
		int shellshockTime;
		int shellshockDuration;
		float dofNearStart;
		float dofNearEnd;
		float dofFarStart;
		float dofFarEnd;
		float dofNearBlur;
		float dofFarBlur;
		float dofViewmodelStart;
		float dofViewmodelEnd;
		int hudElemLastAssignedSoundID;
		objective_t objective[16];
		char weaponmodels[128];
		int deltaTime;
		int killCamEntity;
		$15067B6A14D88D7E1E730369692C3A81 hud;
	};

	struct areaParms_t
	{
		const float* mins;
		const float* maxs;
		int* list;
		int count;
		int maxcount;
		int contentmask;
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

	enum TraceHitType
	{
		TRACE_HITTYPE_NONE = 0x0,
		TRACE_HITTYPE_ENTITY = 0x1,
		TRACE_HITTYPE_DYNENT_MODEL = 0x2,
		TRACE_HITTYPE_DYNENT_BRUSH = 0x3,
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

	enum PMoveFlags
	{
		PMF_NONE = 0,
		PMF_DUCKED = BIT(1),
		PMF_MANTLE = BIT(2),
		PMF_LADDER = BIT(3),
		PMF_BACKWARDS_JUMP = BIT(5),
		PMF_LADDER_DOWN = BIT(5) | BIT(3),
		PMF_LEAN = BIT(6),
		PMF_TIME_KNOCKBACK = BIT(8),
		PMF_JUMP_HELD = BIT(10),
		PMF_FROZEN = BIT(11),
		PMF_JUMPING = BIT(14),
		PMF_PRONE = BIT(14) | BIT(0),
		PMF_SPRINTING = BIT(15),
		PMF_TOOK_DAMAGE = BIT(16),
		PMF_MELEEING = BIT(17),
		PMF_RESPAWNED = BIT(19),
		PMF_VEHICLE_ATTACHED = BIT(20)
	};

	enum MapType
	{
		MAPTYPE_NONE = 0x0,
		MAPTYPE_INVALID1 = 0x1,
		MAPTYPE_INVALID2 = 0x2,
		MAPTYPE_2D = 0x3,
		MAPTYPE_3D = 0x4,
		MAPTYPE_CUBE = 0x5,
		MAPTYPE_COUNT = 0x6,
	};

	struct Picmip
	{
		char platform[2];
	};

	struct CardMemory
	{
		int platform[2];
	};

	struct GfxImageLoadDef
	{
		char levelCount;
		char flags;
		short dimensions[3];
		int format;
		int resourceSize;
		char data[1];
	};

	union GfxTexture
	{
		IDirect3DBaseTexture9* basemap;
		IDirect3DTexture9* map;
		IDirect3DVolumeTexture9* volmap;
		IDirect3DCubeTexture9* cubemap;
		GfxImageLoadDef* loadDef;
		void* data;
	};

	struct GfxImage
	{
		MapType mapType;
		GfxTexture texture;
		Picmip picmip;
		bool noPicmip;
		char semantic;
		char track;
		CardMemory cardMemory;
		uint16_t width;
		uint16_t height;
		uint16_t depth;
		char category;
		bool delayLoadPixels;
		const char* name;
	};

	enum file_image_flags_t
	{
		IMG_FLAG_NOPICMIP = BIT(0),
		IMG_FLAG_NOMIPMAPS = BIT(1),
		IMG_FLAG_CUBEMAP = BIT(2),
		IMG_FLAG_VOLMAP = BIT(3),
		IMG_FLAG_STREAMING = BIT(4),
		IMG_FLAG_LEGACY_NORMALS = BIT(5),
		IMG_FLAG_CLAMP_U = BIT(6),
		IMG_FLAG_CLAMP_V = BIT(7),
		IMG_FLAG_DYNAMIC = BIT(16),
		IMG_FLAG_RENDER_TARGET = BIT(17),
		IMG_FLAG_SYSTEMMEM = BIT(18)
	};

	struct GfxImageFileHeader
	{
		char tag[3];
		char version;
		char format;
		char flags;
		short dimensions[3];
		int fileSizeForPicmip[4];
	};

	struct GfxVertexShaderLoadDef
	{
		uint32_t* program;
		uint16_t programSize;
		uint16_t loadForRenderer;
	};

	struct MaterialVertexShaderProgram
	{
		void* vs;
		GfxVertexShaderLoadDef loadDef;
	};

	struct MaterialVertexShader
	{
		const char* name;
		MaterialVertexShaderProgram prog;
	};

	struct GfxPixelShaderLoadDef
	{
		uint32_t* program;
		uint16_t programSize;
		uint16_t loadForRenderer;
	};

	struct MaterialPixelShaderProgram
	{
		void* ps;
		GfxPixelShaderLoadDef loadDef;
	};

	struct MaterialPixelShader
	{
		const char* name;
		MaterialPixelShaderProgram prog;
	};

	struct MaterialConstantDef
	{
		int nameHash;
		char name[12];
		vec4 literal;
	};

	struct GfxStateBits
	{
		uint32_t loadBits[2];
	};

	struct WaterWritable
	{
		float floatTime;
	};

	struct complex_s
	{
		float real;
		float imag;
	};

	struct water_t
	{
		WaterWritable writable;
		complex_s* H0;
		float* wTerm;
		int M;
		int N;
		float Lx;
		float Lz;
		float gravity;
		float windvel;
		float winddir[2];
		float amplitude;
		float codeConstant[4];
		GfxImage* image;
	};

	union MaterialTextureDefInfo
	{
		GfxImage* image;
		water_t* water;
	};

	struct MaterialTextureDef
	{
		uint32_t nameHash;
		char nameStart;
		char nameEnd;
		char samplerState;
		char semantic;
		MaterialTextureDefInfo u;
	};

	struct GfxDrawSurfFields
	{
		uint64_t objectId : 16;
		uint64_t reflectionProbeIndex : 8;
		uint64_t customIndex : 5;
		uint64_t materialSortedIndex : 11;
		uint64_t prepass : 2;
		uint64_t primaryLightIndex : 8;
		uint64_t surfType : 4;
		uint64_t primarySortKey : 6;
		uint64_t unused : 4;
	};

	union GfxDrawSurf
	{
		GfxDrawSurfFields fields;
		uint64_t packed;
	};

	struct MaterialArgumentCodeConst
	{
		uint16_t index;
		char firstRow;
		char rowCount;
	};

	union MaterialArgumentDef
	{
		const float* literalConst;
		MaterialArgumentCodeConst codeConst;
		uint32_t codeSampler;
		uint32_t nameHash;
	};

	struct MaterialShaderArgument
	{
		uint16_t type;
		uint16_t dest;
		MaterialArgumentDef u;
	};

	enum ShaderCodeConstants
	{
		CONST_SRC_CODE_MAYBE_DIRTY_PS_BEGIN = 0x0,
		CONST_SRC_CODE_LIGHT_POSITION = 0x0,
		CONST_SRC_CODE_LIGHT_DIFFUSE = 0x1,
		CONST_SRC_CODE_LIGHT_SPECULAR = 0x2,
		CONST_SRC_CODE_LIGHT_SPOTDIR = 0x3,
		CONST_SRC_CODE_LIGHT_SPOTFACTORS = 0x4,
		CONST_SRC_CODE_NEARPLANE_ORG = 0x5,
		CONST_SRC_CODE_NEARPLANE_DX = 0x6,
		CONST_SRC_CODE_NEARPLANE_DY = 0x7,
		CONST_SRC_CODE_SHADOW_PARMS = 0x8,
		CONST_SRC_CODE_SHADOWMAP_POLYGON_OFFSET = 0x9,
		CONST_SRC_CODE_RENDER_TARGET_SIZE = 0xA,
		CONST_SRC_CODE_LIGHT_FALLOFF_PLACEMENT = 0xB,
		CONST_SRC_CODE_DOF_EQUATION_VIEWMODEL_AND_FAR_BLUR = 0xC,
		CONST_SRC_CODE_DOF_EQUATION_SCENE = 0xD,
		CONST_SRC_CODE_DOF_LERP_SCALE = 0xE,
		CONST_SRC_CODE_DOF_LERP_BIAS = 0xF,
		CONST_SRC_CODE_DOF_ROW_DELTA = 0x10,
		CONST_SRC_CODE_PARTICLE_CLOUD_COLOR = 0x11,
		CONST_SRC_CODE_GAMETIME = 0x12,
		CONST_SRC_CODE_MAYBE_DIRTY_PS_END = 0x13,
		CONST_SRC_CODE_ALWAYS_DIRTY_PS_BEGIN = 0x13,
		CONST_SRC_CODE_PIXEL_COST_FRACS = 0x13,
		CONST_SRC_CODE_PIXEL_COST_DECODE = 0x14,
		CONST_SRC_CODE_FILTER_TAP_0 = 0x15,
		CONST_SRC_CODE_FILTER_TAP_1 = 0x16,
		CONST_SRC_CODE_FILTER_TAP_2 = 0x17,
		CONST_SRC_CODE_FILTER_TAP_3 = 0x18,
		CONST_SRC_CODE_FILTER_TAP_4 = 0x19,
		CONST_SRC_CODE_FILTER_TAP_5 = 0x1A,
		CONST_SRC_CODE_FILTER_TAP_6 = 0x1B,
		CONST_SRC_CODE_FILTER_TAP_7 = 0x1C,
		CONST_SRC_CODE_COLOR_MATRIX_R = 0x1D,
		CONST_SRC_CODE_COLOR_MATRIX_G = 0x1E,
		CONST_SRC_CODE_COLOR_MATRIX_B = 0x1F,
		CONST_SRC_CODE_ALWAYS_DIRTY_PS_END = 0x20,
		CONST_SRC_CODE_NEVER_DIRTY_PS_BEGIN = 0x20,
		CONST_SRC_CODE_SHADOWMAP_SWITCH_PARTITION = 0x20,
		CONST_SRC_CODE_SHADOWMAP_SCALE = 0x21,
		CONST_SRC_CODE_ZNEAR = 0x22,
		CONST_SRC_CODE_SUN_POSITION = 0x23,
		CONST_SRC_CODE_SUN_DIFFUSE = 0x24,
		CONST_SRC_CODE_SUN_SPECULAR = 0x25,
		CONST_SRC_CODE_LIGHTING_LOOKUP_SCALE = 0x26,
		CONST_SRC_CODE_DEBUG_BUMPMAP = 0x27,
		CONST_SRC_CODE_MATERIAL_COLOR = 0x28,
		CONST_SRC_CODE_FOG = 0x29,
		CONST_SRC_CODE_FOG_COLOR = 0x2A,
		CONST_SRC_CODE_GLOW_SETUP = 0x2B,
		CONST_SRC_CODE_GLOW_APPLY = 0x2C,
		CONST_SRC_CODE_COLOR_BIAS = 0x2D,
		CONST_SRC_CODE_COLOR_TINT_BASE = 0x2E,
		CONST_SRC_CODE_COLOR_TINT_DELTA = 0x2F,
		CONST_SRC_CODE_OUTDOOR_FEATHER_PARMS = 0x30,
		CONST_SRC_CODE_ENVMAP_PARMS = 0x31,
		CONST_SRC_CODE_SPOT_SHADOWMAP_PIXEL_ADJUST = 0x32,
		CONST_SRC_CODE_CLIP_SPACE_LOOKUP_SCALE = 0x33,
		CONST_SRC_CODE_CLIP_SPACE_LOOKUP_OFFSET = 0x34,
		CONST_SRC_CODE_PARTICLE_CLOUD_MATRIX = 0x35,
		CONST_SRC_CODE_DEPTH_FROM_CLIP = 0x36,
		CONST_SRC_CODE_CODE_MESH_ARG_0 = 0x37,
		CONST_SRC_CODE_CODE_MESH_ARG_1 = 0x38,
		CONST_SRC_CODE_CODE_MESH_ARG_LAST = 0x38,
		CONST_SRC_CODE_BASE_LIGHTING_COORDS = 0x39,
		CONST_SRC_CODE_NEVER_DIRTY_PS_END = 0x3A,
		CONST_SRC_CODE_COUNT_FLOAT4 = 0x3A,
		CONST_SRC_FIRST_CODE_MATRIX = 0x3A,
		CONST_SRC_CODE_WORLD_MATRIX = 0x3A,
		CONST_SRC_CODE_INVERSE_WORLD_MATRIX = 0x3B,
		CONST_SRC_CODE_TRANSPOSE_WORLD_MATRIX = 0x3C,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_WORLD_MATRIX = 0x3D,
		CONST_SRC_CODE_VIEW_MATRIX = 0x3E,
		CONST_SRC_CODE_INVERSE_VIEW_MATRIX = 0x3F,
		CONST_SRC_CODE_TRANSPOSE_VIEW_MATRIX = 0x40,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_VIEW_MATRIX = 0x41,
		CONST_SRC_CODE_PROJECTION_MATRIX = 0x42,
		CONST_SRC_CODE_INVERSE_PROJECTION_MATRIX = 0x43,
		CONST_SRC_CODE_TRANSPOSE_PROJECTION_MATRIX = 0x44,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_PROJECTION_MATRIX = 0x45,
		CONST_SRC_CODE_WORLD_VIEW_MATRIX = 0x46,
		CONST_SRC_CODE_INVERSE_WORLD_VIEW_MATRIX = 0x47,
		CONST_SRC_CODE_TRANSPOSE_WORLD_VIEW_MATRIX = 0x48,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX = 0x49,
		CONST_SRC_CODE_VIEW_PROJECTION_MATRIX = 0x4A,
		CONST_SRC_CODE_INVERSE_VIEW_PROJECTION_MATRIX = 0x4B,
		CONST_SRC_CODE_TRANSPOSE_VIEW_PROJECTION_MATRIX = 0x4C,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_VIEW_PROJECTION_MATRIX = 0x4D,
		CONST_SRC_CODE_WORLD_VIEW_PROJECTION_MATRIX = 0x4E,
		CONST_SRC_CODE_INVERSE_WORLD_VIEW_PROJECTION_MATRIX = 0x4F,
		CONST_SRC_CODE_TRANSPOSE_WORLD_VIEW_PROJECTION_MATRIX = 0x50,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_WORLD_VIEW_PROJECTION_MATRIX = 0x51,
		CONST_SRC_CODE_SHADOW_LOOKUP_MATRIX = 0x52,
		CONST_SRC_CODE_INVERSE_SHADOW_LOOKUP_MATRIX = 0x53,
		CONST_SRC_CODE_TRANSPOSE_SHADOW_LOOKUP_MATRIX = 0x54,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_SHADOW_LOOKUP_MATRIX = 0x55,
		CONST_SRC_CODE_WORLD_OUTDOOR_LOOKUP_MATRIX = 0x56,
		CONST_SRC_CODE_INVERSE_WORLD_OUTDOOR_LOOKUP_MATRIX = 0x57,
		CONST_SRC_CODE_TRANSPOSE_WORLD_OUTDOOR_LOOKUP_MATRIX = 0x58,
		CONST_SRC_CODE_INVERSE_TRANSPOSE_WORLD_OUTDOOR_LOOKUP_MATRIX = 0x59,
		CONST_SRC_TOTAL_COUNT = 0x5A,
		CONST_SRC_NONE = 0x5B,
	};

	struct MaterialStreamRouting
	{
		char source;
		char dest;
	};

	struct MaterialVertexStreamRouting
	{
		MaterialStreamRouting data[16];
		void* decl[16];
	};

	struct MaterialVertexDeclaration
	{
		char streamCount;
		bool hasOptionalSource;
		bool isLoaded;
		MaterialVertexStreamRouting routing;
	};

#pragma pack(push, 4)
	struct MaterialPass
	{
		MaterialVertexDeclaration* vertexDecl;
		MaterialVertexShader* vertexShader;
		MaterialPixelShader* pixelShader;
		char perPrimArgCount;
		char perObjArgCount;
		char stableArgCount;
		char customSamplerFlags;
		MaterialShaderArgument* args;
	};
#pragma pack(pop)

	struct MaterialTechnique
	{
		const char* name;
		uint16_t flags;
		uint16_t passCount;
		MaterialPass passArray[1];
	};

	enum MaterialWorldVertexFormat : uint8_t
	{
		MTL_WORLDVERT_TEX_1_NRM_1 = 0x0,
		MTL_WORLDVERT_TEX_2_NRM_1 = 0x1,
		MTL_WORLDVERT_TEX_2_NRM_2 = 0x2,
		MTL_WORLDVERT_TEX_3_NRM_1 = 0x3,
		MTL_WORLDVERT_TEX_3_NRM_2 = 0x4,
		MTL_WORLDVERT_TEX_3_NRM_3 = 0x5,
		MTL_WORLDVERT_TEX_4_NRM_1 = 0x6,
		MTL_WORLDVERT_TEX_4_NRM_2 = 0x7,
		MTL_WORLDVERT_TEX_4_NRM_3 = 0x8,
		MTL_WORLDVERT_TEX_5_NRM_1 = 0x9,
		MTL_WORLDVERT_TEX_5_NRM_2 = 0xA,
		MTL_WORLDVERT_TEX_5_NRM_3 = 0xB,
	};

	struct MaterialTechniqueSet
	{
		char* name;
		MaterialWorldVertexFormat worldVertFormat;
		bool hasBeenUploaded;
		char unused[1];
		MaterialTechniqueSet* remappedTechniqueSet;
		MaterialTechnique* techniques[34];
	};

#pragma pack(push, 4)
	struct MaterialInfo
	{
		const char* name;
		char gameFlags;
		char sortKey;
		char textureAtlasRowCount;
		char textureAtlasColumnCount;
		GfxDrawSurf drawSurf;
		uint32_t surfaceTypeBits;
		uint16_t hashIndex;
	};
#pragma pack(pop)

	enum MaterialTechniqueType
	{
		TECHNIQUE_DEPTH_PREPASS = 0x0,
		TECHNIQUE_BUILD_FLOAT_Z = 0x1,
		TECHNIQUE_BUILD_SHADOWMAP_DEPTH = 0x2,
		TECHNIQUE_BUILD_SHADOWMAP_COLOR = 0x3,
		TECHNIQUE_UNLIT = 0x4,
		TECHNIQUE_EMISSIVE = 0x5,
		TECHNIQUE_EMISSIVE_SHADOW = 0x6,
		TECHNIQUE_LIT_BEGIN = 0x7,
		TECHNIQUE_LIT = 0x7,
		TECHNIQUE_LIT_SUN = 0x8,
		TECHNIQUE_LIT_SUN_SHADOW = 0x9,
		TECHNIQUE_LIT_SPOT = 0xA,
		TECHNIQUE_LIT_SPOT_SHADOW = 0xB,
		TECHNIQUE_LIT_OMNI = 0xC,
		TECHNIQUE_LIT_OMNI_SHADOW = 0xD,
		TECHNIQUE_LIT_INSTANCED = 0xE,
		TECHNIQUE_LIT_INSTANCED_SUN = 0xF,
		TECHNIQUE_LIT_INSTANCED_SUN_SHADOW = 0x10,
		TECHNIQUE_LIT_INSTANCED_SPOT = 0x11,
		TECHNIQUE_LIT_INSTANCED_SPOT_SHADOW = 0x12,
		TECHNIQUE_LIT_INSTANCED_OMNI = 0x13,
		TECHNIQUE_LIT_INSTANCED_OMNI_SHADOW = 0x14,
		TECHNIQUE_LIT_END = 0x15,
		TECHNIQUE_LIGHT_SPOT = 0x15,
		TECHNIQUE_LIGHT_OMNI = 0x16,
		TECHNIQUE_LIGHT_SPOT_SHADOW = 0x17,
		TECHNIQUE_FAKELIGHT_NORMAL = 0x18,
		TECHNIQUE_FAKELIGHT_VIEW = 0x19,
		TECHNIQUE_SUNLIGHT_PREVIEW = 0x1A,
		TECHNIQUE_CASE_TEXTURE = 0x1B,
		TECHNIQUE_WIREFRAME_SOLID = 0x1C,
		TECHNIQUE_WIREFRAME_SHADED = 0x1D,
		TECHNIQUE_SHADOWCOOKIE_CASTER = 0x1E,
		TECHNIQUE_SHADOWCOOKIE_RECEIVER = 0x1F,
		TECHNIQUE_DEBUG_BUMPMAP = 0x20,
		TECHNIQUE_DEBUG_BUMPMAP_INSTANCED = 0x21,
	};

	struct infoParm_t
	{
		const char* name;
		int clearSolid;
		int surfaceFlags;
		int contents;
		int toolFlags;
	};

	struct Material
	{
		MaterialInfo info;
		char stateBitsEntry[34];
		char textureCount;
		char constantCount;
		char stateBitsCount;
		char stateFlags;
		char cameraRegion;
		MaterialTechniqueSet* techniqueSet;
		MaterialTextureDef* textureTable;
		MaterialConstantDef* constantTable;
		GfxStateBits* stateBitsTable;
	};

	struct cplane_s
	{
		vec3 normal;
		float dist;
		char type;
		char signbits;
		char pad[2];
	};

#pragma pack(push, 2)
	struct cbrushside_t
	{
		cplane_s* plane;
		uint32_t materialNum;
		short firstAdjacentSideOffset;
		char edgeCount;
	};
#pragma pack(pop)

	struct DObjAnimMat
	{
		float quat[4];
		float trans[3];
		float transWeight;
	};

	struct XSurfaceVertexInfo
	{
		short vertCount[4];
		uint16_t* vertsBlend;
	};

	struct GfxPointVertex
	{
		vec3 position;
		char color[4];
	};

	union PackedTexCoords
	{
		uint32_t packed;
	};

	struct alignas(4) ShowCollisionBrushPt
	{
		vec3 position;
		short sideIndex[3];
	};

	struct winding_t
	{
		int numpoints;
		float p[4][3];
	};

	struct GfxPackedVertex
	{
		vec3 position;
		float binormalSign;
		GfxColor color;
		PackedTexCoords texCoord;
		PackedUnitVec normal;
		PackedUnitVec tangent;
	};

	struct XSurfaceCollisionAabb
	{
		uint16_t mins[3];
		uint16_t maxs[3];
	};

	struct XSurfaceCollisionNode
	{
		XSurfaceCollisionAabb aabb;
		uint16_t childBeginIndex;
		uint16_t childCount;
	};

	struct XSurfaceCollisionLeaf
	{
		uint16_t triangleBeginIndex;
	};

	struct XSurfaceCollisionTree
	{
		float trans[3];
		float scale[3];
		uint32_t nodeCount;
		XSurfaceCollisionNode* nodes;
		uint32_t leafCount;
		XSurfaceCollisionLeaf* leafs;
	};

	struct XRigidVertList
	{
		uint16_t boneOffset;
		uint16_t vertCount;
		uint16_t triOffset;
		uint16_t triCount;
		XSurfaceCollisionTree* collisionTree;
	};

	struct XSurface
	{
		char tileMode;
		bool deformed;
		uint16_t vertCount;
		uint16_t triCount;
		char zoneHandle;
		uint16_t baseTriIndex;
		uint16_t baseVertIndex;
		uint16_t* triIndices;
		XSurfaceVertexInfo vertInfo;
		GfxPackedVertex* verts0;
		uint32_t vertListCount;
		XRigidVertList* vertList;
		int partBits[4];
	};

	struct BrushWrapper
	{
		vec3 mins;
		int contents;
		vec3 maxs;
		uint32_t numsides;
		cbrushside_t* sides;
		short axialMaterialNum[2][3];
		char* baseAdjacentSide;
		short firstAdjacentSideOffsets[2][3];
		char edgeCount[2][3];
		int totalEdgeCount;
		cplane_s* planes;
	};

	struct PhysMass
	{
		float centerOfMass[3];
		float momentsOfInertia[3];
		float productsOfInertia[3];
	};

	struct PhysGeomInfo
	{
		BrushWrapper* brush;
		int type;
		float orientation[3][3];
		float offset[3];
		float halfLengths[3];
	};

	struct PhysGeomList
	{
		uint32_t count;
		PhysGeomInfo* geoms;
		PhysMass mass;
	};

	struct XBoneInfo
	{
		float bounds[2][3];
		float offset[3];
		float radiusSquared;
	};

	struct XModelHighMipBounds
	{
		float mins[3];
		float maxs[3];
	};

	struct XModelStreamInfo
	{
		XModelHighMipBounds* highMipBounds;
	};

	struct XModelCollTri_s
	{
		float plane[4];
		float svec[4];
		float tvec[4];
	};

	struct XModelCollSurf_s
	{
		XModelCollTri_s* collTris;
		int numCollTris;
		float mins[3];
		float maxs[3];
		int boneIdx;
		int contents;
		int surfFlags;
	};

	struct XModelLodInfo
	{
		float dist;
		uint16_t numsurfs;
		uint16_t surfIndex;
		int partBits[4];
		char lod;
		char smcIndexPlusOne;
		char smcAllocBits;
		char unused;
	};

#pragma pack(push, 4)
	struct PhysPreset
	{
		const char* name;
		int type;
		float mass;
		float bounce;
		float friction;
		float bulletForceScale;
		float explosiveForceScale;
		const char* sndAliasPrefix;
		float piecesSpreadFraction;
		float piecesUpwardVelocity;
		char tempDefaultToCylinder;
	};
#pragma pack(pop)

	struct XModel
	{
		const char* name;
		char numBones;
		char numRootBones;
		uint8_t numsurfs;
		char lodRampType;
		uint16_t* boneNames;
		char* parentList;
		short* quats;
		float* trans;
		char* partClassification;
		DObjAnimMat* baseMat;
		XSurface* surfs;
		Material** materialHandles;
		XModelLodInfo lodInfo[4];
		XModelCollSurf_s* collSurfs;
		int numCollSurfs;
		int contents;
		XBoneInfo* boneInfo;
		float radius;
		float mins[3];
		float maxs[3];
		short numLods;
		short collLod;
		XModelStreamInfo streamInfo;
		int memUsage;
		char flags;
		bool bad;
		PhysPreset* physPreset;
		PhysGeomList* physGeoms;
	};

	union XAnimIndices
	{
		char* _1;
		uint16_t* _2;
		void* data;
	};

	union XAnimDynamicFrames
	{
		char (*_1)[3];
		uint16_t (*_2)[3];
	};

	union XAnimDynamicIndices
	{
		char _1[1];
		uint16_t _2[1];
	};

#pragma pack(push, 4)
	struct XAnimPartTransFrames
	{
		float mins[3];
		float size[3];
		XAnimDynamicFrames frames;
		XAnimDynamicIndices indices;
	};

	union XAnimPartTransData
	{
		XAnimPartTransFrames frames;
		float frame0[3];
	};

	struct XAnimPartTrans
	{
		uint16_t size;
		char smallTrans;
		ALIGN(2) XAnimPartTransData u;
	};

	struct XAnimDeltaPartQuatDataFrames
	{
		short (*frames)[2];
		XAnimDynamicIndices indices;
	};

	union XAnimDeltaPartQuatData
	{
		XAnimDeltaPartQuatDataFrames frames;
		short frame0[2];
	};

	struct XAnimDeltaPartQuat
	{
		uint16_t size;
		alignas(4) XAnimDeltaPartQuatData u;
	};

	struct XAnimDeltaPart
	{
		XAnimPartTrans* trans;
		XAnimDeltaPartQuat* quat;
	};

	struct XAnimNotifyInfo
	{
		uint16_t name;
		float time;
	};

	enum XAnimPartType
	{
		PART_TYPE_NO_QUAT = 0x0,
		PART_TYPE_SIMPLE_QUAT = 0x1,
		PART_TYPE_NORMAL_QUAT = 0x2,
		PART_TYPE_PRECISION_QUAT = 0x3,
		PART_TYPE_SIMPLE_QUAT_NO_SIZE = 0x4,
		PART_TYPE_NORMAL_QUAT_NO_SIZE = 0x5,
		PART_TYPE_PRECISION_QUAT_NO_SIZE = 0x6,
		PART_TYPE_SMALL_TRANS = 0x7,
		PART_TYPE_TRANS = 0x8,
		PART_TYPE_TRANS_NO_SIZE = 0x9,
		PART_TYPE_NO_TRANS = 0xA,
		PART_TYPE_ALL = 0xB,
		PART_TYPE_COUNT = 0xC,
	};
#pragma pack(pop)

	struct XAnimParts
	{
		const char* name;
		uint16_t dataByteCount;
		uint16_t dataShortCount;
		uint16_t dataIntCount;
		uint16_t randomDataByteCount;
		uint16_t randomDataIntCount;
		uint16_t numframes;
		char bLoop;
		char bDelta;
		char boneCount[12];
		char notifyCount;
		char assetType;
		uint32_t randomDataShortCount;
		uint32_t indexCount;
		float framerate;
		float frequency;
		uint16_t* names;
		char* dataByte;
		short* dataShort;
		int* dataInt;
		short* randomDataShort;
		char* randomDataByte;
		int* randomDataInt;
		XAnimIndices indices;
		XAnimNotifyInfo* notify;
		XAnimDeltaPart* deltaPart;
	};

	struct GfxStreamingAabbTree
	{
		uint16_t firstItem;
		uint16_t itemCount;
		uint16_t firstChild;
		uint16_t childCount;
		float mins[3];
		float maxs[3];
	};

	struct GfxWorldStreamInfo
	{
		int aabbTreeCount;
		GfxStreamingAabbTree* aabbTrees;
		int leafRefCount;
		int* leafRefs;
	};

	struct GfxWorldVertexData
	{
		GfxWorldVertex* vertices;
		IDirect3DVertexBuffer9* world;
	};

	struct GfxWorldVertexLayerData
	{
		char* data;
		IDirect3DVertexBuffer9* layer;
	};

	struct SunLightParseParams
	{
		char name[64];
		float ambientScale;
		float ambientColor[3];
		float diffuseFraction;
		float sunLight;
		float sunColor[3];
		float diffuseColor[3];
		char diffuseColorHasBeenSet;
		float angles[3];
	};

	struct alignas(4) GfxLightImage
	{
		GfxImage* image;
		char samplerState;
	};

	struct GfxLightDef
	{
		const char* name;
		GfxLightImage attenuation;
		int lmapLookupStart;
	};

	struct GfxLight
	{
		char type;
		char canUseShadowMap;
		char unused[2];
		float color[3];
		float dir[3];
		float origin[3];
		float radius;
		float cosHalfFovOuter;
		float cosHalfFovInner;
		int exponent;
		uint32_t spotShadowIndex;
		GfxLightDef* def;
	};

	struct GfxReflectionProbe
	{
		float origin[3];
		GfxImage* reflectionImage;
	};

	struct GfxWorldDpvsPlanes
	{
		int cellCount;
		cplane_s* planes;
		uint16_t* nodes;
		uint32_t* sceneEntCellBits;
	};

	struct GfxAabbTree
	{
		float mins[3];
		float maxs[3];
		uint16_t childCount;
		uint16_t surfaceCount;
		uint16_t startSurfIndex;
		uint16_t surfaceCountNoDecal;
		uint16_t startSurfIndexNoDecal;
		uint16_t smodelIndexCount;
		uint16_t* smodelIndexes;
		int childrenOffset;
	};

	struct GfxPortalWritable
	{
		char isQueued;
		char isAncestor;
		char recursionDepth;
		char hullPointCount;
		float (*hullPoints)[2];
		GfxPortal* queuedParent;
	};

	struct DpvsPlane
	{
		float coeffs[4];
		char side[3];
		char pad;
	};

	struct GfxPortal
	{
		GfxPortalWritable writable;
		DpvsPlane plane;
		GfxCell* cell;
		float (*vertices)[3];
		char vertexCount;
		float hullAxis[2][3];
	};

	struct GfxCell
	{
		float mins[3];
		float maxs[3];
		int aabbTreeCount;
		GfxAabbTree* aabbTree;
		int portalCount;
		GfxPortal* portals;
		int cullGroupCount;
		int* cullGroups;
		char reflectionProbeCount;
		char* reflectionProbes;
	};

	struct GfxLightmapArray
	{
		GfxImage* primary;
		GfxImage* secondary;
	};

	struct GfxLightGridEntry
	{
		uint16_t colorsIndex;
		char primaryLightIndex;
		char needsTrace;
	};

	struct GfxLightGridColors
	{
		char rgb[56][3];
	};

	struct GfxLightGrid
	{
		char hasLightRegions;
		uint32_t sunPrimaryLightIndex;
		uint16_t mins[3];
		uint16_t maxs[3];
		uint32_t rowAxis;
		uint32_t colAxis;
		uint16_t* rowDataStart;
		uint32_t rawRowDataSize;
		char* rawRowData;
		uint32_t entryCount;
		GfxLightGridEntry* entries;
		uint32_t colorCount;
		GfxLightGridColors* colors;
	};

	struct GfxBrushModelWritable
	{
		float mins[3];
		float maxs[3];
	};

	struct alignas(4) GfxBrushModel
	{
		GfxBrushModelWritable writable;
		float bounds[2][3];
		uint16_t surfaceCount;
		uint16_t startSurfIndex;
		uint16_t surfaceCountNoDecal;
	};

	struct MaterialMemory
	{
		Material* material;
		int memory;
	};

	struct Glyph
	{
		uint16_t letter;
		char x0;
		char y0;
		char dx;
		char pixelWidth;
		char pixelHeight;
		float s0;
		float t0;
		float s1;
		float t1;
	};

	struct Font_s
	{
		const char* fontName;
		int pixelHeight;
		int glyphCount;
		Material* material;
		Material* glowMaterial;
		Glyph* glyphs;
	};

	struct Fonts_t
	{
		Font_s* bigFont;
		Font_s* smallFont;
		Font_s* consoleFont;
		Font_s* boldFont;
		Font_s* normalFont;
		Font_s* extraBigFont;
		Font_s* objectiveFont;
	};

	enum DemoType
	{
		DEMO_TYPE_NONE = 0x0,
		DEMO_TYPE_CLIENT = 0x1,
		DEMO_TYPE_SERVER = 0x2,
	};

	enum CubemapShot
	{
		CUBEMAPSHOT_NONE = 0x0,
		CUBEMAPSHOT_RIGHT = 0x1,
		CUBEMAPSHOT_LEFT = 0x2,
		CUBEMAPSHOT_BACK = 0x3,
		CUBEMAPSHOT_FRONT = 0x4,
		CUBEMAPSHOT_UP = 0x5,
		CUBEMAPSHOT_DOWN = 0x6,
		CUBEMAPSHOT_COUNT = 0x7,
	};

	enum trType_t
	{
		TR_STATIONARY = 0x0,
		TR_INTERPOLATE = 0x1,
		TR_LINEAR = 0x2,
		TR_LINEAR_STOP = 0x3,
		TR_SINE = 0x4,
		TR_GRAVITY = 0x5,
		TR_ACCELERATE = 0x6,
		TR_DECELERATE = 0x7,
		TR_PHYSICS = 0x8,
		TR_FIRST_RAGDOLL = 0x9,
		TR_RAGDOLL = 0x9,
		TR_RAGDOLL_GRAVITY = 0xA,
		TR_RAGDOLL_INTERPOLATE = 0xB,
		TR_LAST_RAGDOLL = 0xB,
	};

	struct trajectory_t
	{
		trType_t trType;
		int trTime;
		int trDuration;
		vec3 trBase;
		vec3 trDelta;
	};

	struct LerpEntityStateEarthquake
	{
		float scale;
		float radius;
		int duration;
	};

	struct LerpEntityStateLoopFx
	{
		float cullDist;
		int period;
	};

	struct LerpEntityStateCustomExplode
	{
		int startTime;
	};

	struct LerpEntityStateTurret
	{
		float gunAngles[3];
	};

	struct LerpEntityStateExplosion
	{
		float innerRadius;
		float magnitude;
	};

	struct LerpEntityStateBulletHit
	{
		float start[3];
	};

	struct LerpEntityStatePrimaryLight
	{
		char colorAndExp[4];
		float intensity;
		float radius;
		float cosHalfFovOuter;
		float cosHalfFovInner;
	};

	struct LerpEntityStatePlayer
	{
		float leanf;
		int movementDir;
	};

	struct LerpEntityStateVehicle
	{
		float bodyPitch;
		float bodyRoll;
		float steerYaw;
		int materialTime;
		float gunPitch;
		float gunYaw;
		int teamAndOwnerIndex;
	};

	struct LerpEntityStateMissile
	{
		int launchTime;
	};

	struct LerpEntityStateSoundBlend
	{
		float lerp;
	};

	struct LerpEntityStateExplosionJolt
	{
		float innerRadius;
		float impulse[3];
	};

	struct LerpEntityStatePhysicsJitter
	{
		float innerRadius;
		float minDisplacement;
		float maxDisplacement;
	};

	struct LerpEntityStateAnonymous
	{
		int data[7];
	};

	union LerpEntityStateTypeUnion
	{
		LerpEntityStateTurret turret;
		LerpEntityStateLoopFx loopFx;
		LerpEntityStatePrimaryLight primaryLight;
		LerpEntityStatePlayer player;
		LerpEntityStateVehicle vehicle;
		LerpEntityStateMissile missile;
		LerpEntityStateSoundBlend soundBlend;
		LerpEntityStateBulletHit bulletHit;
		LerpEntityStateEarthquake earthquake;
		LerpEntityStateCustomExplode customExplode;
		LerpEntityStateExplosion explosion;
		LerpEntityStateExplosionJolt explosionJolt;
		LerpEntityStatePhysicsJitter physicsJitter;
		LerpEntityStateAnonymous anonymous;
	};

	struct LerpEntityState
	{
		int eFlags;
		trajectory_t pos;
		trajectory_t apos;
		LerpEntityStateTypeUnion u;
	};

	enum entityType_t
	{
		ET_GENERAL = 0x0,
		ET_PLAYER = 0x1,
		ET_PLAYER_CORPSE = 0x2,
		ET_ITEM = 0x3,
		ET_MISSILE = 0x4,
		ET_INVISIBLE = 0x5,
		ET_SCRIPTMOVER = 0x6,
		ET_SOUND_BLEND = 0x7,
		ET_FX = 0x8,
		ET_LOOP_FX = 0x9,
		ET_PRIMARY_LIGHT = 0xA,
		ET_MG42 = 0xB,
		ET_HELICOPTER = 0xC,
		ET_PLANE = 0xD,
		ET_VEHICLE = 0xE,
		ET_VEHICLE_COLLMAP = 0xF,
		ET_VEHICLE_CORPSE = 0x10,
		ET_EVENTS = 0x11,
		ET_MOVER = 0x99
	};

	struct entityState_s
	{
		int number;
		enum entityType_t eType;
		LerpEntityState lerp;
		int time2;
		int otherEntityNum;
		int attackerEntityNum;
		int groundEntityNum;
		int loopSound;
		int surfType;
		int index;
		int clientNum;
		int iHeadIcon;
		int iHeadIconTeam;
		int solid;
		uint32_t eventParm;
		int eventSequence;
		int events[4];
		int eventParms[4];
		int weapon;
		int weaponModel;
		int legsAnim;
		int torsoAnim;
		int indexUnion1;
		int indexUnion2;
		float fTorsoPitch;
		float fWaistPitch;
		uint32_t partBits[4];
	};

	enum team_t
	{
		TEAM_FREE = 0x0,
		TEAM_AXIS = 0x1,
		TEAM_ALLIES = 0x2,
		TEAM_SPECTATOR = 0x3,
		TEAM_NUM_TEAMS = 0x4,
	};

	struct clientState_s
	{
		int clientIndex;
		team_t team;
		int modelindex;
		int attachModelIndex[6];
		int attachTagIndex[6];
		char name[16];
		float maxSprintTimeMultiplier;
		int rank;
		int prestige;
		int perks;
		int attachedVehEntNum;
		int attachedVehSlotIndex;
	};

	struct snapshot_s
	{
		int snapFlags;
		int ping;
		int serverTime;
		playerState_s ps;
		int numEntities;
		int numClients;
		entityState_s entities[512];
		clientState_s clients[64];
		int serverCommandSequence;
	};

	struct sunflare_t
	{
		char hasValidData;
		Material* spriteMaterial;
		Material* flareMaterial;
		float spriteSize;
		float flareMinSize;
		float flareMinDot;
		float flareMaxSize;
		float flareMaxDot;
		float flareMaxAlpha;
		int flareFadeInTime;
		int flareFadeOutTime;
		float blindMinDot;
		float blindMaxDot;
		float blindMaxDarken;
		int blindFadeInTime;
		int blindFadeOutTime;
		float glareMinDot;
		float glareMaxDot;
		float glareMaxLighten;
		int glareFadeInTime;
		int glareFadeOutTime;
		float sunFxPosition[3];
	};

	struct XModelDrawInfo
	{
		uint16_t lod;
		uint16_t surfId;
	};

	struct GfxSceneDynModel
	{
		XModelDrawInfo info;
		uint16_t dynEntId;
	};

	struct BModelDrawInfo
	{
		uint16_t surfId;
	};

	struct GfxSceneDynBrush
	{
		BModelDrawInfo info;
		uint16_t dynEntId;
	};

	struct GfxShadowGeometry
	{
		uint16_t surfaceCount;
		uint16_t smodelCount;
		uint16_t* sortedSurfIndex;
		uint16_t* smodelIndex;
	};

	struct GfxLightRegionAxis
	{
		float dir[3];
		float midPoint;
		float halfSize;
	};

	struct GfxLightRegionHull
	{
		float kdopMidPoint[9];
		float kdopHalfSize[9];
		uint32_t axisCount;
		GfxLightRegionAxis* axis;
	};

	struct GfxLightRegion
	{
		uint32_t hullCount;
		GfxLightRegionHull* hulls;
	};

	struct GfxStaticModelInst
	{
		float mins[3];
		float maxs[3];
		GfxColor groundLighting;
	};

	struct srfTriangles_t
	{
		int vertexLayerData;
		int firstVertex;
		uint16_t vertexCount;
		uint16_t triCount;
		int baseIndex;
	};

	struct GfxSurface
	{
		srfTriangles_t tris;
		Material* material;
		char lightmapIndex;
		char reflectionProbeIndex;
		char primaryLightIndex;
		char flags;
		float bounds[2][3];
	};

	struct GfxCullGroup
	{
		vec3 mins;
		vec3 maxs;
		int surfaceCount;
		int startSurfIndex;
	};

	struct GfxPackedPlacement
	{
		vec3 origin;
		vec3 axis[3];
		float scale;
	};

	struct alignas(4) GfxStaticModelDrawInst
	{
		float cullDist;
		GfxPackedPlacement placement;
		XModel* model;
		uint16_t smodelCacheIndex[4];
		char reflectionProbeIndex;
		char primaryLightIndex;
		uint16_t lightingHandle;
		char flags;
	};

	struct GfxWorldDpvsStatic
	{
		uint32_t smodelCount;
		uint32_t staticSurfaceCount;
		uint32_t staticSurfaceCountNoDecal;
		uint32_t litSurfsBegin;
		uint32_t litSurfsEnd;
		uint32_t decalSurfsBegin;
		uint32_t decalSurfsEnd;
		uint32_t emissiveSurfsBegin;
		uint32_t emissiveSurfsEnd;
		uint32_t smodelVisDataCount;
		uint32_t surfaceVisDataCount;
		char* smodelVisData[3];
		char* surfaceVisData[3];
		uint32_t* lodData;
		uint16_t* sortedSurfIndex;
		GfxStaticModelInst* smodelInsts;
		GfxSurface* surfaces;
		GfxCullGroup* cullGroups;
		GfxStaticModelDrawInst* smodelDrawInsts;
		GfxDrawSurf* surfaceMaterials;
		uint32_t* surfaceCastsSunShadow;
		volatile int usageCount;
	};

	struct GfxWorldDpvsDynamic
	{
		uint32_t dynEntClientWordCount[2];
		uint32_t dynEntClientCount[2];
		uint32_t* dynEntCellBits[2];
		char* dynEntVisData[2][3];
	};

	struct GfxWorld
	{
		const char* name;
		const char* baseName;
		int planeCount;
		int nodeCount;
		int indexCount;
		uint16_t* indices;
		int surfaceCount;
		GfxWorldStreamInfo streamInfo;
		int skySurfCount;
		int* skyStartSurfs;
		GfxImage* skyImage;
		char skySamplerState;
		uint32_t vertexCount;
		GfxWorldVertexData vd;
		uint32_t vertexLayerDataSize;
		GfxWorldVertexLayerData vld;
		SunLightParseParams sunParse;
		GfxLight* sunLight;
		float sunColorFromBsp[3];
		uint32_t sunPrimaryLightIndex;
		uint32_t primaryLightCount;
		int cullGroupCount;
		uint32_t reflectionProbeCount;
		GfxReflectionProbe* reflectionProbes;
		GfxTexture* reflectionProbeTextures;
		GfxWorldDpvsPlanes dpvsPlanes;
		int cellBitsCount;
		GfxCell* cells;
		int lightmapCount;
		GfxLightmapArray* lightmaps;
		GfxLightGrid lightGrid;
		GfxTexture* lightmapPrimaryTextures;
		GfxTexture* lightmapSecondaryTextures;
		int modelCount;
		GfxBrushModel* models;
		float mins[3];
		float maxs[3];
		uint32_t checksum;
		int materialMemoryCount;
		MaterialMemory* materialMemory;
		sunflare_t sun;
		float outdoorLookupMatrix[4][4];
		GfxImage* outdoorImage;
		uint32_t* cellCasterBits;
		GfxSceneDynModel* sceneDynModel;
		GfxSceneDynBrush* sceneDynBrush;
		uint32_t* primaryLightEntityShadowVis;
		uint32_t* primaryLightDynEntShadowVis[2];
		char* nonSunPrimaryLightForModelDynEnt;
		GfxShadowGeometry* shadowGeom;
		GfxLightRegion* lightRegion;
		GfxWorldDpvsStatic dpvs;
		GfxWorldDpvsDynamic dpvsDyn;
	};

	struct cStaticModelWritable
	{
		uint16_t nextModelInWorldSector;
	};

	struct cStaticModel_s
	{
		cStaticModelWritable writable;
		XModel* xmodel;
		float origin[3];
		float invScaledAxis[3][3];
		float absmin[3];
		float absmax[3];
	};

	struct dmaterial_t
	{
		char material[64];
		int surfaceFlags;
		int contentFlags;
	};

	enum SurfaceFlags
	{
		SURF_NODAMAGE = 0x1,
		SURF_SLICK = 0x2,
		SURF_SKY = 0x4,
		SURF_LADDER = 0x8,
		SURF_NOIMPACT = 0x10,
		SURF_NOMARKS = 0x20,
		SURF_NODRAW = 0x80,
		SURF_NOPENETRATE = 0x100,
		SURF_NOLIGHTMAP = 0x400,
		SURF_NOSTEPS = 0x2000,
		SURF_NONSOLID = 0x4000,
		SURF_NODLIGHT = 0x20000,
		SURF_NOCASTSHADOW = 0x40000,
		SURF_MANTLEON = 0x2000000,
		SURF_MANTLEOVER = 0x4000000,
		SURF_PORTAL = 0x80000000,
		SURF_BARK = 0x100000,
		SURF_BRICK = 0x200000,
		SURF_CARPET = 0x300000,
		SURF_CLOTH = 0x400000,
		SURF_CONCRETE = 0x500000,
		SURF_DIRT = 0x600000,
		SURF_FLESH = 0x700000,
		SURF_FOLIAGE = 0x800000,
		SURF_GLASS = 0x900000,
		SURF_GRASS = 0xA00000,
		SURF_GRAVEL = 0xB00000,
		SURF_ICE = 0xC00000,
		SURF_METAL = 0xD00000,
		SURF_MUD = 0xE00000,
		SURF_PAPER = 0xF00000,
		SURF_PLASTER = 0x1000000,
		SURF_ROCK = 0x1100000,
		SURF_SAND = 0x1200000,
		SURF_SNOW = 0x1300000,
		SURF_WATER = 0x1400000,
		SURF_WOOD = 0x1500000,
		SURF_ASPHALT = 0x1600000,
		SURF_CERAMIC = 0x1700000,
		SURF_PLASTIC = 0x1800000,
		SURF_RUBBER = 0x1900000,
		SURF_CUSHION = 0x1A00000,
		SURF_FRUIT = 0x1B00000,
		SURF_PAINTEDMETAL = 0x1C00000,
	};

	struct cNode_t
	{
		cplane_s* plane;
		short children[2];
	};

#pragma pack(push, 4)
	struct cLeaf_t
	{
		uint16_t firstCollAabbIndex;
		uint16_t collAabbCount;
		int brushContents;
		int terrainContents;
		float mins[3];
		float maxs[3];
		int leafBrushNode;
		short cluster;
	};
#pragma pack(pop)

	struct cLeafBrushNodeLeaf_t
	{
		uint16_t* brushes;
	};

	struct cLeafBrushNodeChildren_t
	{
		float dist;
		float range;
		uint16_t childOffset[2];
	};

	union cLeafBrushNodeData_t
	{
		cLeafBrushNodeLeaf_t leaf;
		cLeafBrushNodeChildren_t children;
	};

	struct cLeafBrushNode_s
	{
		char axis;
		short leafBrushCount;
		int contents;
		cLeafBrushNodeData_t data;
	};

	struct CollisionBorder
	{
		float distEq[3];
		float zBase;
		float zSlope;
		float start;
		float length;
	};

	struct CollisionPartition
	{
		char triCount;
		char borderCount;
		int firstTri;
		CollisionBorder* borders;
	};

	union CollisionAabbTreeIndex
	{
		int firstChildIndex;
		int partitionIndex;
	};

	struct CollisionAabbTree
	{
		float origin[3];
		float halfSize[3];
		uint16_t materialIndex;
		uint16_t childCount;
		CollisionAabbTreeIndex u;
	};

	struct cmodel_t
	{
		float mins[3];
		float maxs[3];
		float radius;
		cLeaf_t leaf;
	};

#pragma pack(push, 16)
	struct cbrush_t
	{
		float mins[3];
		int contents;
		float maxs[3];
		uint32_t numsides;
		cbrushside_t* sides;
		short axialMaterialNum[2][3];
		char* baseAdjacentSide;
		short firstAdjacentSideOffsets[2][3];
		char edgeCount[2][3];
		short colorCounter;
		short cmBrushIndex;
		float distFromCam;
		short cmSubmodelIndex;
		bool isSubmodel;
		bool pad;
	};
#pragma pack(pop)

	struct Bounds
	{
		vec3 midPoint;
		vec3 halfSize;
	};

	struct TriggerModel
	{
		int contents;
		uint16_t hullCount;
		uint16_t firstHull;
	};

	struct TriggerHull
	{
		Bounds bounds;
		int contents;
		uint16_t slabCount;
		uint16_t firstSlab;
	};

	struct TriggerSlab
	{
		float dir[3];
		float midPoint;
		float halfSize;
	};

	struct MapTriggers
	{
		uint32_t count;
		TriggerModel* models;
		uint32_t hullCount;
		TriggerHull* hulls;
		uint32_t slabCount;
		TriggerSlab* slabs;
	};

	struct MapEnts
	{
		const char* name;
		char* entityString;
		int numEntityChars;
		MapTriggers trigger;
	};

	struct GfxPlacement
	{
		float quat[4];
		float origin[3];
	};

	struct FxEffectDef_Placeholder
	{
		const char* name;
	};

	struct DynEntityDef
	{
		int type;
		GfxPlacement pose;
		XModel* xModel;
		uint16_t brushModel;
		uint16_t physicsBrushModel;
		FxEffectDef_Placeholder* destroyFx;
		void* destroyPieces; // XModelPieces
		PhysPreset* physPreset;
		int health;
		PhysMass mass;
		int contents;
	};

	struct clipMap_t
	{
		const char* name;
		int isInUse;
		int planeCount;
		cplane_s* planes;
		uint32_t numStaticModels;
		cStaticModel_s* staticModelList;
		uint32_t numMaterials;
		dmaterial_t* materials;
		uint32_t numBrushSides;
		cbrushside_t* brushsides;
		uint32_t numBrushEdges;
		char* brushEdges;
		uint32_t numNodes;
		cNode_t* nodes;
		uint32_t numLeafs;
		cLeaf_t* leafs;
		uint32_t leafbrushNodesCount;
		cLeafBrushNode_s* leafbrushNodes;
		uint32_t numLeafBrushes;
		uint16_t* leafbrushes;
		uint32_t numLeafSurfaces;
		uint32_t* leafsurfaces;
		uint32_t vertCount;
		float (*verts)[3];
		int triCount;
		uint16_t* triIndices;
		char* triEdgeIsWalkable;
		int borderCount;
		CollisionBorder* borders;
		int partitionCount;
		CollisionPartition* partitions;
		int aabbTreeCount;
		CollisionAabbTree* aabbTrees;
		uint32_t numSubModels;
		cmodel_t* cmodels;
		uint16_t numBrushes;
		cbrush_t* brushes;
		int numClusters;
		int clusterBytes;
		char* visibility;
		int vised;
		MapEnts* mapEnts;
		cbrush_t* box_brush;
		cmodel_t box_model;
		uint16_t dynEntCount[2];
		DynEntityDef* dynEntDefList[2];
		void* dynEntPoseList[2];   // DynEntityPose
		void* dynEntClientList[2]; // DynEntityClient
		void* dynEntCollList[2];   // DynEntityColl
		uint32_t checksum;
	};

	struct RawFile
	{
		const char* name;
		int len;
		const char* buffer;
	};

	struct ComPrimaryLight
	{
		char type;
		char canUseShadowMap;
		char exponent;
		char unused;
		float color[3];
		float dir[3];
		float origin[3];
		float radius;
		float cosHalfFovOuter;
		float cosHalfFovInner;
		float cosHalfFovExpanded;
		float rotationLimit;
		float translationLimit;
		const char* defName;
	};

	struct ComWorld
	{
		const char* name;
		int isInUse;
		uint32_t primaryLightCount;
		ComPrimaryLight* primaryLights;
	};

	struct FxSpawnDefLooping
	{
		int intervalMsec;
		int count;
	};

	struct FxIntRange
	{
		int base;
		int amplitude;
	};

	struct FxSpawnDefOneShot
	{
		FxIntRange count;
	};

	union FxSpawnDef
	{
		FxSpawnDefLooping looping;
		FxSpawnDefOneShot oneShot;
	};

	struct FxFloatRange
	{
		float base;
		float amplitude;
	};

	struct FxElemAtlas
	{
		char behavior;
		char index;
		char fps;
		char loopCount;
		char colIndexBits;
		char rowIndexBits;
		short entryCount;
	};

	struct FxElemVec3Range
	{
		float base[3];
		float amplitude[3];
	};

	struct FxElemVelStateInFrame
	{
		FxElemVec3Range velocity;
		FxElemVec3Range totalDelta;
	};

	const struct FxElemVelStateSample
	{
		FxElemVelStateInFrame local;
		FxElemVelStateInFrame world;
	};

	struct FxElemVisualState
	{
		char color[4];
		float rotationDelta;
		float rotationTotal;
		float size[2];
		float scale;
	};

	const struct FxElemVisStateSample
	{
		FxElemVisualState base;
		FxElemVisualState amplitude;
	};

	union FxEffectDefRef
	{
		FxEffectDef* handle;
		const char* name;
	};

	union FxElemVisuals
	{
		const void* anonymous;
		Material* material;
		XModel* model;
		FxEffectDefRef effectDef;
		const char* soundName;
	};

	struct FxElemMarkVisuals
	{
		Material* materials[2];
	};

	union FxElemDefVisuals
	{
		FxElemMarkVisuals* markArray;
		FxElemVisuals* array;
		FxElemVisuals instance;
	};

	struct FxTrailVertex
	{
		float pos[2];
		float normal[2];
		float texCoord;
	};

	struct FxTrailDef
	{
		int scrollTimeMsec;
		int repeatDist;
		int splitDist;
		int vertCount;
		FxTrailVertex* verts;
		int indCount;
		uint16_t* inds;
	};

	const struct FxElemDef
	{
		int flags;
		FxSpawnDef spawn;
		FxFloatRange spawnRange;
		FxFloatRange fadeInRange;
		FxFloatRange fadeOutRange;
		float spawnFrustumCullRadius;
		FxIntRange spawnDelayMsec;
		FxIntRange lifeSpanMsec;
		FxFloatRange spawnOrigin[3];
		FxFloatRange spawnOffsetRadius;
		FxFloatRange spawnOffsetHeight;
		FxFloatRange spawnAngles[3];
		FxFloatRange angularVelocity[3];
		FxFloatRange initialRotation;
		FxFloatRange gravity;
		FxFloatRange reflectionFactor;
		FxElemAtlas atlas;
		char elemType;
		char visualCount;
		char velIntervalCount;
		char visStateIntervalCount;
		FxElemVelStateSample* velSamples;
		FxElemVisStateSample* visSamples;
		FxElemDefVisuals visuals;
		float collMins[3];
		float collMaxs[3];
		FxEffectDefRef effectOnImpact;
		FxEffectDefRef effectOnDeath;
		FxEffectDefRef effectEmitted;
		FxFloatRange emitDist;
		FxFloatRange emitDistVariance;
		FxTrailDef* trailDef;
		char sortOrder;
		char lightingFrac;
		char useItemClip;
		char unused[1];
	};

	struct FxEffectDef
	{
		const char* name;
		int flags;
		int totalSize;
		int msecLoopingLife;
		int elemDefCountLooping;
		int elemDefCountOneShot;
		int elemDefCountEmission;
		FxElemDef* elemDefs;
	};

	enum FxElemType : uint8_t
	{
		FX_ELEM_TYPE_SPRITE_BILLBOARD = 0x0,
		FX_ELEM_TYPE_SPRITE_ORIENTED = 0x1,
		FX_ELEM_TYPE_TAIL = 0x2,
		FX_ELEM_TYPE_TRAIL = 0x3,
		FX_ELEM_TYPE_CLOUD = 0x4,
		FX_ELEM_TYPE_MODEL = 0x5,
		FX_ELEM_TYPE_OMNI_LIGHT = 0x6,
		FX_ELEM_TYPE_SPOT_LIGHT = 0x7,
		FX_ELEM_TYPE_SOUND = 0x8,
		FX_ELEM_TYPE_DECAL = 0x9,
		FX_ELEM_TYPE_RUNNER = 0xA,
		FX_ELEM_TYPE_COUNT = 0xB,
		FX_ELEM_TYPE_LAST_SPRITE = 0x3,
		FX_ELEM_TYPE_LAST_DRAWN = 0x7,
	};

	struct pathlink_s
	{
		float fDist;
		uint16_t nodeNum;
		char disconnectCount;
		char negotiationLink;
		char ubBadPlaceCount[4];
	};

	enum nodeType
	{
		NODE_BADNODE = 0x0,
		NODE_PATHNODE = 0x1,
		NODE_COVER_STAND = 0x2,
		NODE_COVER_CROUCH = 0x3,
		NODE_COVER_CROUCH_WINDOW = 0x4,
		NODE_COVER_PRONE = 0x5,
		NODE_COVER_RIGHT = 0x6,
		NODE_COVER_LEFT = 0x7,
		NODE_COVER_WIDE_RIGHT = 0x8,
		NODE_COVER_WIDE_LEFT = 0x9,
		NODE_CONCEALMENT_STAND = 0xA,
		NODE_CONCEALMENT_CROUCH = 0xB,
		NODE_CONCEALMENT_PRONE = 0xC,
		NODE_REACQUIRE = 0xD,
		NODE_BALCONY = 0xE,
		NODE_SCRIPTED = 0xF,
		NODE_NEGOTIATION_BEGIN = 0x10,
		NODE_NEGOTIATION_END = 0x11,
		NODE_TURRET = 0x12,
		NODE_GUARD = 0x13,
		NODE_NUMTYPES = 0x14,
		NODE_DONTLINK = 0x14,
	};

	struct pathnode_constant_t
	{
		nodeType type;
		uint16_t spawnflags;
		uint16_t targetname;
		uint16_t script_linkName;
		uint16_t script_noteworthy;
		uint16_t target;
		uint16_t animscript;
		int animscriptfunc;
		float vOrigin[3];
		float fAngle;
		float forward[2];
		float fRadius;
		float minUseDistSq;
		short wOverlapNode[2];
		short wChainId;
		short wChainDepth;
		short wChainParent;
		uint16_t totalLinkCount;
		pathlink_s* Links;
	};

	struct pathnode_dynamic_t
	{
		void* pOwner;
		int iFreeTime;
		int iValidTime[3];
		int inPlayerLOSTime;
		short wLinkCount;
		short wOverlapCount;
		short turretEntNumber;
		short userCount;
	};

	struct pathnode_transient_t
	{
		int iSearchFrame;
		pathnode_t* pNextOpen;
		pathnode_t* pPrevOpen;
		pathnode_t* pParent;
		float fCost;
		float fHeuristic;
		float costFactor;
	};

	struct pathnode_t
	{
		pathnode_constant_t constant;
		pathnode_dynamic_t dynamic;
		pathnode_transient_t transient;
	};

	struct pathbasenode_t
	{
		float vOrigin[3];
		uint32_t type;
	};

	struct pathnode_tree_nodes_t
	{
		int nodeCount;
		uint16_t* nodes;
	};

	union pathnode_tree_info_t
	{
		pathnode_tree_t* child[2];
		pathnode_tree_nodes_t s;
	};

	struct pathnode_tree_t
	{
		int axis;
		float dist;
		pathnode_tree_info_t u;
	};

	struct PathData
	{
		uint32_t nodeCount;
		pathnode_t* nodes;
		pathbasenode_t* basenodes;
		uint32_t chainNodeCount;
		uint16_t* chainNodeForNode;
		uint16_t* nodeForChainNode;
		int visBytes;
		char* pathVis;
		int nodeTreeCount;
		pathnode_tree_t* nodeTree;
	};

	struct GameWorldSp
	{
		const char* name;
		PathData path;
	};

	struct GameWorldMp
	{
		const char* name;
	};

	struct EntHandle
	{
		uint16_t number;
		uint16_t infoIndex;
	};

	struct entityShared_t
	{
		char linked;
		char bmodel;
		char svFlags;
		int clientMask[2];
		char inuse;
		int broadcastTime;
		vec3 mins;
		vec3 maxs;
		int contents;
		vec3 absmin;
		vec3 absmax;
		vec3 currentOrigin;
		vec3 currentAngles;
		EntHandle ownerNum;
		int eventTime;
	};

	enum sessionState_t
	{
		SESS_STATE_PLAYING = 0x0,
		SESS_STATE_DEAD = 0x1,
		SESS_STATE_SPECTATOR = 0x2,
		SESS_STATE_INTERMISSION = 0x3,
	};

	enum clientConnected_t
	{
		CON_DISCONNECTED = 0x0,
		CON_CONNECTING = 0x1,
		CON_CONNECTED = 0x2,
	};

	struct playerTeamState_t
	{
		int location;
	};

	enum weapType_t
	{
		WEAPTYPE_BULLET = 0x0,
		WEAPTYPE_GRENADE = 0x1,
		WEAPTYPE_PROJECTILE = 0x2,
		WEAPTYPE_BINOCULARS = 0x3,
		WEAPTYPE_NUM = 0x4,
	};

	enum weapClass_t
	{
		WEAPCLASS_RIFLE = 0x0,
		WEAPCLASS_MG = 0x1,
		WEAPCLASS_SMG = 0x2,
		WEAPCLASS_SPREAD = 0x3,
		WEAPCLASS_PISTOL = 0x4,
		WEAPCLASS_GRENADE = 0x5,
		WEAPCLASS_ROCKETLAUNCHER = 0x6,
		WEAPCLASS_TURRET = 0x7,
		WEAPCLASS_NON_PLAYER = 0x8,
		WEAPCLASS_ITEM = 0x9,
		WEAPCLASS_NUM = 0xA,
	};

	enum PenetrateType
	{
		PENETRATE_TYPE_NONE = 0x0,
		PENETRATE_TYPE_SMALL = 0x1,
		PENETRATE_TYPE_MEDIUM = 0x2,
		PENETRATE_TYPE_LARGE = 0x3,
		PENETRATE_TYPE_COUNT = 0x4,
	};

	enum ImpactType
	{
		IMPACT_TYPE_NONE = 0x0,
		IMPACT_TYPE_BULLET_SMALL = 0x1,
		IMPACT_TYPE_BULLET_LARGE = 0x2,
		IMPACT_TYPE_BULLET_AP = 0x3,
		IMPACT_TYPE_SHOTGUN = 0x4,
		IMPACT_TYPE_GRENADE_BOUNCE = 0x5,
		IMPACT_TYPE_GRENADE_EXPLODE = 0x6,
		IMPACT_TYPE_ROCKET_EXPLODE = 0x7,
		IMPACT_TYPE_PROJECTILE_DUD = 0x8,
		IMPACT_TYPE_COUNT = 0x9,
	};

	enum weapInventoryType_t
	{
		WEAPINVENTORY_PRIMARY = 0x0,
		WEAPINVENTORY_OFFHAND = 0x1,
		WEAPINVENTORY_ITEM = 0x2,
		WEAPINVENTORY_ALTMODE = 0x3,
		WEAPINVENTORYCOUNT = 0x4,
	};

	enum weapFireType_t
	{
		WEAPON_FIRETYPE_FULLAUTO = 0x0,
		WEAPON_FIRETYPE_SINGLESHOT = 0x1,
		WEAPON_FIRETYPE_BURSTFIRE2 = 0x2,
		WEAPON_FIRETYPE_BURSTFIRE3 = 0x3,
		WEAPON_FIRETYPE_BURSTFIRE4 = 0x4,
		WEAPON_FIRETYPECOUNT = 0x5,
	};

	enum OffhandClass
	{
		OFFHAND_CLASS_NONE = 0x0,
		OFFHAND_CLASS_FRAG_GRENADE = 0x1,
		OFFHAND_CLASS_SMOKE_GRENADE = 0x2,
		OFFHAND_CLASS_FLASH_GRENADE = 0x3,
		OFFHAND_CLASS_COUNT = 0x4,
	};

	enum weapStance_t
	{
		WEAPSTANCE_STAND = 0x0,
		WEAPSTANCE_DUCK = 0x1,
		WEAPSTANCE_PRONE = 0x2,
		WEAPSTANCE_NUM = 0x3,
	};

	enum activeReticleType_t
	{
		VEH_ACTIVE_RETICLE_NONE = 0x0,
		VEH_ACTIVE_RETICLE_PIP_ON_A_STICK = 0x1,
		VEH_ACTIVE_RETICLE_BOUNCING_DIAMOND = 0x2,
		VEH_ACTIVE_RETICLE_COUNT = 0x3,
	};

	enum weaponIconRatioType_t
	{
		WEAPON_ICON_RATIO_1TO1 = 0x0,
		WEAPON_ICON_RATIO_2TO1 = 0x1,
		WEAPON_ICON_RATIO_4TO1 = 0x2,
		WEAPON_ICON_RATIO_COUNT = 0x3,
	};

	enum ammoCounterClipType_t
	{
		AMMO_COUNTER_CLIP_NONE = 0x0,
		AMMO_COUNTER_CLIP_MAGAZINE = 0x1,
		AMMO_COUNTER_CLIP_SHORTMAGAZINE = 0x2,
		AMMO_COUNTER_CLIP_SHOTGUN = 0x3,
		AMMO_COUNTER_CLIP_ROCKET = 0x4,
		AMMO_COUNTER_CLIP_BELTFED = 0x5,
		AMMO_COUNTER_CLIP_ALTWEAPON = 0x6,
		AMMO_COUNTER_CLIP_COUNT = 0x7,
	};

	enum weapOverlayReticle_t
	{
		WEAPOVERLAYRETICLE_NONE = 0x0,
		WEAPOVERLAYRETICLE_CROSSHAIR = 0x1,
		WEAPOVERLAYRETICLE_NUM = 0x2,
	};

	enum WeapOverlayInteface_t
	{
		WEAPOVERLAYINTERFACE_NONE = 0x0,
		WEAPOVERLAYINTERFACE_JAVELIN = 0x1,
		WEAPOVERLAYINTERFACE_TURRETSCOPE = 0x2,
		WEAPOVERLAYINTERFACECOUNT = 0x3,
	};

	enum weapProjExposion_t
	{
		WEAPPROJEXP_GRENADE = 0x0,
		WEAPPROJEXP_ROCKET = 0x1,
		WEAPPROJEXP_FLASHBANG = 0x2,
		WEAPPROJEXP_NONE = 0x3,
		WEAPPROJEXP_DUD = 0x4,
		WEAPPROJEXP_SMOKE = 0x5,
		WEAPPROJEXP_HEAVY = 0x6,
		WEAPPROJEXP_NUM = 0x7,
	};

	enum WeapStickinessType
	{
		WEAPSTICKINESS_NONE = 0x0,
		WEAPSTICKINESS_ALL = 0x1,
		WEAPSTICKINESS_GROUND = 0x2,
		WEAPSTICKINESS_GROUND_WITH_YAW = 0x3,
		WEAPSTICKINESS_COUNT = 0x4,
	};

	enum guidedMissileType_t
	{
		MISSILE_GUIDANCE_NONE = 0x0,
		MISSILE_GUIDANCE_SIDEWINDER = 0x1,
		MISSILE_GUIDANCE_HELLFIRE = 0x2,
		MISSILE_GUIDANCE_JAVELIN = 0x3,
		MISSILE_GUIDANCE_COUNT = 0x4,
	};

	struct clientSession_t
	{
		sessionState_t sessionState;
		int forceSpectatorClient;
		int killCamEntity;
		int status_icon;
		int archiveTime;
		int score;
		int deaths;
		int kills;
		int assists;
		uint16_t scriptPersId;
		clientConnected_t connected;
		usercmd_s cmd;
		usercmd_s oldcmd;
		int localClient;
		int predictItemPickup;
		char newnetname[16];
		int maxHealth;
		int enterTime;
		playerTeamState_t teamState;
		int voteCount;
		int teamVoteCount;
		float moveSpeedScaleMultiplier;
		int viewmodelIndex;
		int noSpectate;
		int teamInfo;
		clientState_s cs;
		int psOffsetTime;
	};

	struct gclient_s
	{
		playerState_s ps;
		clientSession_t sess;
		int spectatorClient;
		int noclip;
		int ufo;
		int bFrozen;
		int lastCmdTime;
		int buttons;
		int oldbuttons;
		int latched_buttons;
		int buttonsSinceLastFrame;
		vec3 oldOrigin;
		float fGunPitch;
		float fGunYaw;
		int damage_blood;
		vec3 damage_from;
		int damage_fromWorld;
		int accurateCount;
		int accuracy_shots;
		int accuracy_hits;
		int inactivityTime;
		int inactivityWarning;
		int lastVoiceTime;
		int switchTeamTime;
		float currentAimSpreadScale;
		gentity_s* persistantPowerup;
		int portalID;
		int dropWeaponTime;
		int sniperRifleFiredTime;
		float sniperRifleMuzzleYaw;
		int PCSpecialPickedUpCount;
		EntHandle useHoldEntity;
		int useHoldTime;
		int useButtonDone;
		int iLastCompassPlayerInfoEnt;
		int compassPingTime;
		int damageTime;
		float v_dmg_roll;
		float v_dmg_pitch;
		vec3 swayViewAngles;
		vec3 swayOffset;
		vec3 swayAngles;
		vec3 vLastMoveAng;
		float fLastIdleFactor;
		vec3 vGunOffset;
		vec3 vGunSpeed;
		int weapIdleTime;
		int lastServerTime;
		int lastSpawnTime;
		uint32_t lastWeapon;
		bool previouslyFiring;
		bool previouslyUsingNightVision;
		bool previouslySprinting;
		int hasRadar;
		int lastStand;
		int lastStandTime;
	};

	struct turretInfo_s
	{
		int inuse;
		int flags;
		int fireTime;
		float arcmin[2];
		float arcmax[2];
		float dropPitch;
		int stance;
		int prevStance;
		int fireSndDelay;
		float userOrigin[3];
		float playerSpread;
		float pitchCap;
		int triggerDown;
		char fireSnd;
		char fireSndPlayer;
		char stopSnd;
		char stopSndPlayer;
	};

	struct item_ent_t
	{
		int ammoCount;
		int clipAmmoCount;
		int index;
	};

	struct alignas(4) trigger_ent_t
	{
		int threshold;
		int accumulate;
		int timestamp;
		int singleUserEntIndex;
		bool requireLookAt;
	};

	struct mover_ent_t
	{
		float decelTime;
		float aDecelTime;
		float speed;
		float aSpeed;
		float midTime;
		float aMidTime;
		float pos1[3];
		float pos2[3];
		float pos3[3];
		float apos1[3];
		float apos2[3];
		float apos3[3];
	};

	struct corpse_ent_t
	{
		int deathAnimStartTime;
	};

	enum MissileStage
	{
		MISSILESTAGE_SOFTLAUNCH = 0x0,
		MISSILESTAGE_ASCENT = 0x1,
		MISSILESTAGE_DESCENT = 0x2,
	};

	enum MissileFlightMode
	{
		MISSILEFLIGHTMODE_TOP = 0x0,
		MISSILEFLIGHTMODE_DIRECT = 0x1,
	};

	struct missile_ent_t
	{
		float time;
		int timeOfBirth;
		float travelDist;
		float surfaceNormal[3];
		team_t team;
		float curvature[3];
		float targetOffset[3];
		MissileStage stage;
		MissileFlightMode flightMode;
	};

	union $4C7580D783CB81EAAF8D9BB4061D1D71
	{
		item_ent_t item[2];
		trigger_ent_t trigger;
		mover_ent_t mover;
		corpse_ent_t corpse;
		missile_ent_t missile;
	};

	struct tagInfo_s
	{
		gentity_s* parent;
		gentity_s* next;
		uint16_t name;
		int index;
		float axis[4][3];
		float parentInvAxis[4][3];
	};

	struct gentity_s
	{
		entityState_s s;
		entityShared_t r;
		gclient_s* client;
		turretInfo_s* pTurretInfo;
		scr_vehicle_s* scr_vehicle;
		uint16_t model;
		char physicsObject;
		char takedamage;
		char active;
		char nopickup;
		char handler;
		char team;
		uint16_t classname;
		uint16_t target;
		uint16_t targetname;
		uint32_t attachIgnoreCollision;
		int spawnflags;
		int flags;
		int eventTime;
		int freeAfterEvent;
		int unlinkAfterEvent;
		int clipmask;
		int processedFrame;
		EntHandle parent;
		int nextthink;
		int health;
		int maxHealth;
		int damage;
		int count;
		gentity_s* chain;
		$4C7580D783CB81EAAF8D9BB4061D1D71 ___u30;
		EntHandle missileTargetEnt;
		tagInfo_s* tagInfo;
		gentity_s* tagChildren;
		uint16_t attachModelNames[19];
		uint16_t attachTagNames[19];
		int useCount;
		gentity_s* nextFree;
	};

	struct WeaponDef
	{
		const char* szInternalName;
		const char* szDisplayName;
		const char* szOverlayName;
		XModel* gunXModel[16];
		XModel* handXModel;
		const char* szXAnims[33];
		const char* szModeName;
		uint16_t hideTags[8];
		uint16_t notetrackSoundMapKeys[16];
		uint16_t notetrackSoundMapValues[16];
		int playerAnimType;
		weapType_t weapType;
		weapClass_t weapClass;
		PenetrateType penetrateType;
		ImpactType impactType;
		weapInventoryType_t inventoryType;
		weapFireType_t fireType;
		OffhandClass offhandClass;
		weapStance_t stance;
		FxEffectDef* viewFlashEffect;
		FxEffectDef* worldFlashEffect;
		snd_alias_list_t* pickupSound;
		snd_alias_list_t* pickupSoundPlayer;
		snd_alias_list_t* ammoPickupSound;
		snd_alias_list_t* ammoPickupSoundPlayer;
		snd_alias_list_t* projectileSound;
		snd_alias_list_t* pullbackSound;
		snd_alias_list_t* pullbackSoundPlayer;
		snd_alias_list_t* fireSound;
		snd_alias_list_t* fireSoundPlayer;
		snd_alias_list_t* fireLoopSound;
		snd_alias_list_t* fireLoopSoundPlayer;
		snd_alias_list_t* fireStopSound;
		snd_alias_list_t* fireStopSoundPlayer;
		snd_alias_list_t* fireLastSound;
		snd_alias_list_t* fireLastSoundPlayer;
		snd_alias_list_t* emptyFireSound;
		snd_alias_list_t* emptyFireSoundPlayer;
		snd_alias_list_t* meleeSwipeSound;
		snd_alias_list_t* meleeSwipeSoundPlayer;
		snd_alias_list_t* meleeHitSound;
		snd_alias_list_t* meleeMissSound;
		snd_alias_list_t* rechamberSound;
		snd_alias_list_t* rechamberSoundPlayer;
		snd_alias_list_t* reloadSound;
		snd_alias_list_t* reloadSoundPlayer;
		snd_alias_list_t* reloadEmptySound;
		snd_alias_list_t* reloadEmptySoundPlayer;
		snd_alias_list_t* reloadStartSound;
		snd_alias_list_t* reloadStartSoundPlayer;
		snd_alias_list_t* reloadEndSound;
		snd_alias_list_t* reloadEndSoundPlayer;
		snd_alias_list_t* detonateSound;
		snd_alias_list_t* detonateSoundPlayer;
		snd_alias_list_t* nightVisionWearSound;
		snd_alias_list_t* nightVisionWearSoundPlayer;
		snd_alias_list_t* nightVisionRemoveSound;
		snd_alias_list_t* nightVisionRemoveSoundPlayer;
		snd_alias_list_t* altSwitchSound;
		snd_alias_list_t* altSwitchSoundPlayer;
		snd_alias_list_t* raiseSound;
		snd_alias_list_t* raiseSoundPlayer;
		snd_alias_list_t* firstRaiseSound;
		snd_alias_list_t* firstRaiseSoundPlayer;
		snd_alias_list_t* putawaySound;
		snd_alias_list_t* putawaySoundPlayer;
		snd_alias_list_t** bounceSound;
		FxEffectDef* viewShellEjectEffect;
		FxEffectDef* worldShellEjectEffect;
		FxEffectDef* viewLastShotEjectEffect;
		FxEffectDef* worldLastShotEjectEffect;
		Material* reticleCenter;
		Material* reticleSide;
		int iReticleCenterSize;
		int iReticleSideSize;
		int iReticleMinOfs;
		activeReticleType_t activeReticleType;
		float vStandMove[3];
		float vStandRot[3];
		float vDuckedOfs[3];
		float vDuckedMove[3];
		float vDuckedRot[3];
		float vProneOfs[3];
		float vProneMove[3];
		float vProneRot[3];
		float fPosMoveRate;
		float fPosProneMoveRate;
		float fStandMoveMinSpeed;
		float fDuckedMoveMinSpeed;
		float fProneMoveMinSpeed;
		float fPosRotRate;
		float fPosProneRotRate;
		float fStandRotMinSpeed;
		float fDuckedRotMinSpeed;
		float fProneRotMinSpeed;
		XModel* worldModel[16];
		XModel* worldClipModel;
		XModel* rocketModel;
		XModel* knifeModel;
		XModel* worldKnifeModel;
		Material* hudIcon;
		weaponIconRatioType_t hudIconRatio;
		Material* ammoCounterIcon;
		weaponIconRatioType_t ammoCounterIconRatio;
		ammoCounterClipType_t ammoCounterClip;
		int iStartAmmo;
		const char* szAmmoName;
		int iAmmoIndex;
		const char* szClipName;
		int iClipIndex;
		int iMaxAmmo;
		int iClipSize;
		int shotCount;
		const char* szSharedAmmoCapName;
		int iSharedAmmoCapIndex;
		int iSharedAmmoCap;
		int damage;
		int playerDamage;
		int iMeleeDamage;
		int iDamageType;
		int iFireDelay;
		int iMeleeDelay;
		int meleeChargeDelay;
		int iDetonateDelay;
		int iFireTime;
		int iRechamberTime;
		int iRechamberBoltTime;
		int iHoldFireTime;
		int iDetonateTime;
		int iMeleeTime;
		int meleeChargeTime;
		int iReloadTime;
		int reloadShowRocketTime;
		int iReloadEmptyTime;
		int iReloadAddTime;
		int iReloadStartTime;
		int iReloadStartAddTime;
		int iReloadEndTime;
		int iDropTime;
		int iRaiseTime;
		int iAltDropTime;
		int iAltRaiseTime;
		int quickDropTime;
		int quickRaiseTime;
		int iFirstRaiseTime;
		int iEmptyRaiseTime;
		int iEmptyDropTime;
		int sprintInTime;
		int sprintLoopTime;
		int sprintOutTime;
		int nightVisionWearTime;
		int nightVisionWearTimeFadeOutEnd;
		int nightVisionWearTimePowerUp;
		int nightVisionRemoveTime;
		int nightVisionRemoveTimePowerDown;
		int nightVisionRemoveTimeFadeInStart;
		int fuseTime;
		int aiFuseTime;
		int requireLockonToFire;
		int noAdsWhenMagEmpty;
		int avoidDropCleanup;
		float autoAimRange;
		float aimAssistRange;
		float aimAssistRangeAds;
		float aimPadding;
		float enemyCrosshairRange;
		int crosshairColorChange;
		float moveSpeedScale;
		float adsMoveSpeedScale;
		float sprintDurationScale;
		float fAdsZoomFov;
		float fAdsZoomInFrac;
		float fAdsZoomOutFrac;
		Material* overlayMaterial;
		Material* overlayMaterialLowRes;
		weapOverlayReticle_t overlayReticle;
		WeapOverlayInteface_t overlayInterface;
		float overlayWidth;
		float overlayHeight;
		float fAdsBobFactor;
		float fAdsViewBobMult;
		float fHipSpreadStandMin;
		float fHipSpreadDuckedMin;
		float fHipSpreadProneMin;
		float hipSpreadStandMax;
		float hipSpreadDuckedMax;
		float hipSpreadProneMax;
		float fHipSpreadDecayRate;
		float fHipSpreadFireAdd;
		float fHipSpreadTurnAdd;
		float fHipSpreadMoveAdd;
		float fHipSpreadDuckedDecay;
		float fHipSpreadProneDecay;
		float fHipReticleSidePos;
		int iAdsTransInTime;
		int iAdsTransOutTime;
		float fAdsIdleAmount;
		float fHipIdleAmount;
		float adsIdleSpeed;
		float hipIdleSpeed;
		float fIdleCrouchFactor;
		float fIdleProneFactor;
		float fGunMaxPitch;
		float fGunMaxYaw;
		float swayMaxAngle;
		float swayLerpSpeed;
		float swayPitchScale;
		float swayYawScale;
		float swayHorizScale;
		float swayVertScale;
		float swayShellShockScale;
		float adsSwayMaxAngle;
		float adsSwayLerpSpeed;
		float adsSwayPitchScale;
		float adsSwayYawScale;
		float adsSwayHorizScale;
		float adsSwayVertScale;
		int bRifleBullet;
		int armorPiercing;
		int bBoltAction;
		int aimDownSight;
		int bRechamberWhileAds;
		float adsViewErrorMin;
		float adsViewErrorMax;
		int bCookOffHold;
		int bClipOnly;
		int adsFireOnly;
		int cancelAutoHolsterWhenEmpty;
		int suppressAmmoReserveDisplay;
		int enhanced;
		int laserSightDuringNightvision;
		Material* killIcon;
		weaponIconRatioType_t killIconRatio;
		int flipKillIcon;
		Material* dpadIcon;
		weaponIconRatioType_t dpadIconRatio;
		int bNoPartialReload;
		int bSegmentedReload;
		int iReloadAmmoAdd;
		int iReloadStartAdd;
		const char* szAltWeaponName;
		uint32_t altWeaponIndex;
		int iDropAmmoMin;
		int iDropAmmoMax;
		int blocksProne;
		int silenced;
		int iExplosionRadius;
		int iExplosionRadiusMin;
		int iExplosionInnerDamage;
		int iExplosionOuterDamage;
		float damageConeAngle;
		int iProjectileSpeed;
		int iProjectileSpeedUp;
		int iProjectileSpeedForward;
		int iProjectileActivateDist;
		float projLifetime;
		float timeToAccelerate;
		float projectileCurvature;
		XModel* projectileModel;
		weapProjExposion_t projExplosion;
		FxEffectDef* projExplosionEffect;
		int projExplosionEffectForceNormalUp;
		FxEffectDef* projDudEffect;
		snd_alias_list_t* projExplosionSound;
		snd_alias_list_t* projDudSound;
		int bProjImpactExplode;
		WeapStickinessType stickiness;
		int hasDetonator;
		int timedDetonation;
		int rotate;
		int holdButtonToThrow;
		int freezeMovementWhenFiring;
		float lowAmmoWarningThreshold;
		float parallelBounce[29];
		float perpendicularBounce[29];
		FxEffectDef* projTrailEffect;
		float vProjectileColor[3];
		guidedMissileType_t guidedMissileType;
		float maxSteeringAccel;
		int projIgnitionDelay;
		FxEffectDef* projIgnitionEffect;
		snd_alias_list_t* projIgnitionSound;
		float fAdsAimPitch;
		float fAdsCrosshairInFrac;
		float fAdsCrosshairOutFrac;
		int adsGunKickReducedKickBullets;
		float adsGunKickReducedKickPercent;
		float fAdsGunKickPitchMin;
		float fAdsGunKickPitchMax;
		float fAdsGunKickYawMin;
		float fAdsGunKickYawMax;
		float fAdsGunKickAccel;
		float fAdsGunKickSpeedMax;
		float fAdsGunKickSpeedDecay;
		float fAdsGunKickStaticDecay;
		float fAdsViewKickPitchMin;
		float fAdsViewKickPitchMax;
		float fAdsViewKickYawMin;
		float fAdsViewKickYawMax;
		float fAdsViewKickCenterSpeed;
		float fAdsViewScatterMin;
		float fAdsViewScatterMax;
		float fAdsSpread;
		int hipGunKickReducedKickBullets;
		float hipGunKickReducedKickPercent;
		float fHipGunKickPitchMin;
		float fHipGunKickPitchMax;
		float fHipGunKickYawMin;
		float fHipGunKickYawMax;
		float fHipGunKickAccel;
		float fHipGunKickSpeedMax;
		float fHipGunKickSpeedDecay;
		float fHipGunKickStaticDecay;
		float fHipViewKickPitchMin;
		float fHipViewKickPitchMax;
		float fHipViewKickYawMin;
		float fHipViewKickYawMax;
		float fHipViewKickCenterSpeed;
		float fHipViewScatterMin;
		float fHipViewScatterMax;
		float fightDist;
		float maxDist;
		const char* accuracyGraphName[2];
		float (*accuracyGraphKnots[2])[2];
		float (*originalAccuracyGraphKnots[2])[2];
		int accuracyGraphKnotCount[2];
		int originalAccuracyGraphKnotCount[2];
		int iPositionReloadTransTime;
		float leftArc;
		float rightArc;
		float topArc;
		float bottomArc;
		float accuracy;
		float aiSpread;
		float playerSpread;
		float minTurnSpeed[2];
		float maxTurnSpeed[2];
		float pitchConvergenceTime;
		float yawConvergenceTime;
		float suppressTime;
		float maxRange;
		float fAnimHorRotateInc;
		float fPlayerPositionDist;
		const char* szUseHintString;
		const char* dropHintString;
		int iUseHintStringIndex;
		int dropHintStringIndex;
		float horizViewJitter;
		float vertViewJitter;
		const char* szScript;
		float fOOPosAnimLength[2];
		int minDamage;
		int minPlayerDamage;
		float fMaxDamageRange;
		float fMinDamageRange;
		float destabilizationRateTime;
		float destabilizationCurvatureMax;
		int destabilizeDistance;
		float locationDamageMultipliers[19];
		const char* fireRumble;
		const char* meleeImpactRumble;
		float adsDofStart;
		float adsDofEnd;
	};

	struct weaponParms
	{
		float forward[3];
		float right[3];
		float up[3];
		float muzzleTrace[3];
		float gunForward[3];
		WeaponDef* weapDef;
	};

	struct archivedEntityShared_t
	{
		int svFlags;
		int clientMask[2];
		float absmin[3];
		float absmax[3];
	};

	struct archivedEntity_s
	{
		entityState_s s;
		archivedEntityShared_t r;
	};

	struct cachedSnapshot_t
	{
		int archivedFrame;
		int time;
		int num_entities;
		int first_entity;
		int num_clients;
		int first_client;
		int usesDelta;
	};

	enum netsrc_t
	{
		NS_CLIENT1 = 0x0,
		NS_SERVER = 0x1,
		NS_MAXCLIENTS = 0x1,
		NS_PACKET = 0x2,
	};

	enum netadrtype_t
	{
		NA_BOT = 0x0,
		NA_BAD = 0x1,
		NA_LOOPBACK = 0x2,
		NA_BROADCAST = 0x3,
		NA_IP = 0x4,
		NA_IPX = 0x5,
		NA_BROADCAST_IPX = 0x6,
	};

	struct netadr_t
	{
		netadrtype_t type;
		char ip[4];
		uint16_t port;
		char ipx[10];
	};

	struct netProfilePacket_t
	{
		int iTime;
		int iSize;
		int bFragment;
	};

	struct netProfileStream_t
	{
		netProfilePacket_t packets[60];
		int iCurrPacket;
		int iBytesPerSecond;
		int iLastBPSCalcTime;
		int iCountedPackets;
		int iCountedFragments;
		int iFragmentPercentage;
		int iLargestPacket;
		int iSmallestPacket;
	};

	struct netProfileInfo_t
	{
		netProfileStream_t send;
		netProfileStream_t recieve;
	};

	struct netchan_t
	{
		int outgoingSequence;
		netsrc_t sock;
		int dropped;
		int incomingSequence;
		netadr_t remoteAddress;
		int qport;
		int fragmentSequence;
		int fragmentLength;
		char* fragmentBuffer;
		int fragmentBufferSize;
		int unsentFragments;
		int unsentFragmentStart;
		int unsentLength;
		char* unsentBuffer;
		int unsentBufferSize;
		netProfileInfo_t prof;
	};

	struct clientHeader_t
	{
		int state;
		int sendAsActive;
		int deltaMessage;
		int rateDelayed;
		netchan_t netchan;
		float predictedOrigin[3];
		int predictedOriginServerTime;
	};

	struct svscmd_info_t
	{
		char cmd[1024];
		int time;
		int type;
	};

	struct clientSnapshot_t
	{
		playerState_s ps;
		int num_entities;
		int num_clients;
		int first_entity;
		int first_client;
		int messageSent;
		int messageAcked;
		int messageSize;
		int serverTime;
	};

	struct VoicePacket_t
	{
		char talker;
		char data[256];
		int dataSize;
	};

	struct clientConnection_t
	{
		int qport;
		int clientNum;
		int lastPacketSentTime;
		int lastPacketTime;
		netadr_t serverAddress;
		int connectTime;
		int connectPacketCount;
		char serverMessage[256];
		int challenge;
		int checksumFeed;
		int reliableSequence;
		int reliableAcknowledge;
		char reliableCommands[128][1024];
		int serverMessageSequence;
		int serverCommandSequence;
		int lastExecutedServerCommand;
		char serverCommands[128][1024];
		bool isServerRestarting;
		int lastClientArchiveIndex;
		char demoName[64];
		int demorecording;
		int demoplaying;
		int isTimeDemo;
		int demowaiting;
		int firstDemoFrameSkipped;
		int demofile;
		int timeDemoLog;
		int timeDemoFrames;
		int timeDemoStart;
		int timeDemoPrev;
		int timeDemoBaseTime;
		netchan_t netchan;
		char netchanOutgoingBuffer[2048];
		char netchanIncomingBuffer[131072];
		netProfileInfo_t OOBProf;
		char statPacketsToSend;
		int statPacketSendTime[7];
	};

	struct client_t
	{
		clientHeader_t header;
		const char* dropReason;
		char userinfo[1024];
		svscmd_info_t reliableCommandInfo[128];
		int reliableSequence;
		int reliableAcknowledge;
		int reliableSent;
		int messageAcknowledge;
		int gamestateMessageNum;
		int challenge;
		usercmd_s lastUsercmd;
		int lastClientCommand;
		char lastClientCommandString[1024];
		gentity_s* gentity;
		char name[16];
		int downloading;
		char downloadName[64];
		int download;
		int downloadSize;
		int downloadCount;
		int downloadClientBlock;
		int downloadCurrentBlock;
		int downloadXmitBlock;
		char* downloadBlocks[8];
		int downloadBlockSize[8];
		int downloadEOF;
		int downloadSendTime;
		char downloadURL[256];
		int wwwOk;
		int downloadingWWW;
		int clientDownloadingWWW;
		int wwwFallback;
		int nextReliableTime;
		int lastPacketTime;
		int lastConnectTime;
		int nextSnapshotTime;
		int timeoutCount;
		clientSnapshot_t frames[32];
		int ping;
		int rate;
		int snapshotMsec;
		int snapshotBackoffCount;
		int pureAuthentic;
		char netchanOutgoingBuffer[131072];
		char netchanIncomingBuffer[2048];
		char cdkeyHash[33];
		uint16_t scriptId;
		int bIsTestClient;
		int serverId;
		VoicePacket_t voicePackets[40];
		int voicePacketCount;
		bool muteList[64];
		bool sendVoice;
		char stats[8192];
		char statPacketsReceived;
		bool tempPacketDebugging;
	};

	struct archivedSnapshot_s
	{
		int start;
		int size;
	};

	struct cachedClient_s
	{
		int playerStateExists;
		clientState_s cs;
		playerState_s ps;
	};

	struct alignas(4) challenge_t
	{
		netadr_t adr;
		int challenge;
		int time;
		int pingTime;
		int firstTime;
		int firstPing;
		int connected;
		char cdkeyHash[33];
	};

	struct tempBanSlot_t
	{
		char cdkeyHash[32];
		int banTime;
	};

	struct serverStatic_t
	{
		cachedSnapshot_t cachedSnapshotFrames[512];
		archivedEntity_s cachedSnapshotEntities[16384];
		int initialized;
		int time;
		int snapFlagServerBit;
		client_t clients[64];
		int numSnapshotEntities;
		int numSnapshotClients;
		int nextSnapshotEntities;
		int nextSnapshotClients;
		entityState_s snapshotEntities[172032];
		clientState_s snapshotClients[131072];
		int nextArchivedSnapshotFrames;
		archivedSnapshot_s archivedSnapshotFrames[1200];
		char archivedSnapshotBuffer[33554432];
		int nextArchivedSnapshotBuffer;
		int nextCachedSnapshotEntities;
		int nextCachedSnapshotClients;
		int nextCachedSnapshotFrames;
		cachedClient_s cachedSnapshotClients[4096];
		int nextHeartbeatTime;
		int nextStatusResponseTime;
		challenge_t challenges[1024];
		netadr_t redirectAddress;
		netadr_t authorizeAddress;
		int sv_lastTimeMasterServerCommunicated;
		netProfileInfo_t OOBProf;
		tempBanSlot_t tempBans[16];
		float mapCenter[3];
	};

	struct ServerProfileTimes
	{
		float frameTime;
		float wallClockTime;
	};

	struct svEntity_s
	{
		uint16_t worldSector;
		uint16_t nextEntityInWorldSector;
		archivedEntity_s baseline;
		int numClusters;
		int clusternums[16];
		int lastCluster;
		int linkcontents;
		float linkmin[2];
		float linkmax[2];
	};

	enum serverState_t
	{
		SS_DEAD = 0x0,
		SS_LOADING = 0x1,
		SS_GAME = 0x2,
	};

	struct server_t
	{
		serverState_t state;
		int timeResidual;
		bool inFrame;
		int restarting;
		int start_frameTime;
		int checksumFeed;
		cmodel_t* models[512];
		uint16_t emptyConfigString;
		uint16_t configstrings[2442];
		svEntity_s svEntities[1024];
		gentity_s* gentities;
		int gentitySize;
		int num_entities;
		playerState_s* gameClients;
		int gameClientSize;
		int skelTimeStamp;
		int skelMemPos;
		int bpsWindow[20];
		int bpsWindowSteps;
		int bpsTotalBytes;
		int bpsMaxBytes;
		int ubpsWindow[20];
		int ubpsTotalBytes;
		int ubpsMaxBytes;
		float ucompAve;
		int ucompNum;
		ServerProfileTimes profile;
		volatile float serverFrameTimeMin;
		volatile float serverFrameTimeMax;
		char gametype[64];
		bool killServer;
		const char* killReason;
	};

	struct GfxMatrix
	{
		float m[4][4];
	};

	struct GfxScaledPlacement
	{
		GfxPlacement base;
		float scale;
	};

	struct GfxParticleCloud
	{
		GfxScaledPlacement placement;
		float endpos[3];
		GfxColor color;
		float radius[2];
		uint32_t pad[2];
	};

	struct GfxViewParms
	{
		GfxMatrix viewMatrix;
		GfxMatrix projectionMatrix;
		GfxMatrix viewProjectionMatrix;
		GfxMatrix inverseViewProjectionMatrix;
		float origin[4];
		float axis[3][3];
		float depthHackNearClip;
		float zNear;
		float zFar;
	};

	struct FxCodeMeshData
	{
		uint32_t triCount;
		uint16_t* indices;
		uint16_t argOffset;
		uint16_t argCount;
		uint32_t pad;
	};

	struct GfxVertexBufferState
	{
		volatile int used;
		int total;
		IDirect3DVertexBuffer9* buffer;
		char* verts;
	};

	struct GfxMeshData
	{
		uint32_t indexCount;
		uint32_t totalIndexCount;
		uint16_t* indices;
		GfxVertexBufferState vb;
		uint32_t vertSize;
	};

	union PackedLightingCoords
	{
		uint32_t packed;
		char array[4];
	};

	struct GfxSModelCachedVertex
	{
		float xyz[3];
		GfxColor color;
		PackedTexCoords texCoord;
		PackedUnitVec normal;
		PackedUnitVec tangent;
		PackedLightingCoords baseLighting;
	};

	struct GfxModelLightingPatch
	{
		uint16_t modelLightingIndex;
		char primaryLightWeight;
		char colorsCount;
		char groundLighting[4];
		uint16_t colorsWeight[8];
		uint16_t colorsIndex[8];
	};

	struct alignas(4) GfxModelSurfaceInfo
	{
		DObjAnimMat* baseMat;
		char boneIndex;
		char boneCount;
		uint16_t gfxEntIndex;
		uint16_t lightingHandle;
	};

	union $178D1D161B34F636C03EBC0CA3007D75
	{
		GfxPackedVertex* skinnedVert;
		int oldSkinnedCachedOffset;
	};

	struct GfxModelSkinnedSurface
	{
		int skinnedCachedOffset;
		XSurface* xsurf;
		GfxModelSurfaceInfo info;
		$178D1D161B34F636C03EBC0CA3007D75 ___u3;
	};

	struct GfxBackEndPrimitiveData
	{
		int hasSunDirChanged;
	};

	struct GfxEntity
	{
		uint32_t renderFxFlags;
		float materialTime;
	};

	struct FxMarkMeshData
	{
		uint32_t triCount;
		uint16_t* indices;
		uint16_t modelIndex;
		char modelTypeAndSurf;
		char pad0;
		uint32_t pad1;
	};

	struct GfxFog
	{
		int startTime;
		int finishTime;
		GfxColor color;
		float fogStart;
		float density;
	};

	struct GfxCmdHeader
	{
		uint16_t id;
		uint16_t byteCount;
	};

	struct GfxCmdArray
	{
		char* cmds;
		int usedTotal;
		int usedCritical;
		GfxCmdHeader* lastCmd;
	};

	struct GfxSceneDef
	{
		int time;
		float floatTime;
		float viewOffset[3];
	};

	struct GfxViewport
	{
		int x;
		int y;
		int width;
		int height;
	};

	enum ShadowType
	{
		SHADOW_NONE = 0x0,
		SHADOW_COOKIE = 0x1,
		SHADOW_MAP = 0x2,
	};

	struct GfxDrawSurfListInfo
	{
		GfxDrawSurf* drawSurfs;
		uint32_t drawSurfCount;
		MaterialTechniqueType baseTechType;
		GfxViewInfo* viewInfo;
		float viewOrigin[4];
		GfxLight* light;
		int cameraView;
	};

	struct alignas(16) ShadowCookie
	{
		GfxMatrix shadowLookupMatrix;
		float boxMin[3];
		float boxMax[3];
		GfxViewParms* shadowViewParms;
		float fade;
		uint32_t sceneEntIndex;
		ALIGN(1) GfxDrawSurfListInfo casterInfo;
		ALIGN(1) GfxDrawSurfListInfo receiverInfo;
	};

	struct GfxWindowTarget
	{
		HWND* hwnd;
		void* swapChain;
		int width;
		int height;
	};

	struct alignas(8) DxGlobals
	{
		HINSTANCE* hinst;
		IDirect3D9* d3d9;
		IDirect3DDevice9* device;
		uint32_t adapterIndex;
		bool adapterNativeIsValid;
		int adapterNativeWidth;
		int adapterNativeHeight;
		int adapterFullscreenWidth;
		int adapterFullscreenHeight;
		int depthStencilFormat;
		uint32_t displayModeCount;
		void* displayModes[256];
		const char* resolutionNameTable[257];
		const char* refreshRateNameTable[257];
		char modeText[5120];
		void* fencePool[8];
		uint32_t nextFence;
		int gpuSync;
		int multiSampleType;
		uint32_t multiSampleQuality;
		int sunSpriteSamples;
		void* singleSampleDepthStencilSurface;
		bool deviceLost;
		bool inScene;
		int targetWindowIndex;
		int windowCount;
		GfxWindowTarget windows[1];
		int flushGpuQueryCount;
		void* flushGpuQuery;
		uint64_t gpuSyncDelay;
		uint64_t gpuSyncStart;
		uint64_t gpuSyncEnd;
		bool flushGpuQueryIssued;
		int linearNonMippedMinFilter;
		int linearNonMippedMagFilter;
		int linearMippedMinFilter;
		int linearMippedMagFilter;
		int anisotropicMinFilter;
		int anisotropicMagFilter;
		int linearMippedAnisotropy;
		int anisotropyFor2x;
		int anisotropyFor4x;
		int mipFilterMode;
		uint32_t mipBias;
		void* swapFence;
	};

	struct alignas(16) ShadowCookieList
	{
		ShadowCookie cookies[24];
		uint32_t cookieCount;
	};

	struct PointLightPartition
	{
		GfxLight light;
		GfxDrawSurfListInfo info;
	};

	enum GfxRenderTargetId
	{
		R_RENDERTARGET_SAVED_SCREEN = 0x0,
		R_RENDERTARGET_FRAME_BUFFER = 0x1,
		R_RENDERTARGET_SCENE = 0x2,
		R_RENDERTARGET_RESOLVED_POST_SUN = 0x3,
		R_RENDERTARGET_RESOLVED_SCENE = 0x4,
		R_RENDERTARGET_FLOAT_Z = 0x5,
		R_RENDERTARGET_DYNAMICSHADOWS = 0x6,
		R_RENDERTARGET_PINGPONG_0 = 0x7,
		R_RENDERTARGET_PINGPONG_1 = 0x8,
		R_RENDERTARGET_SHADOWCOOKIE = 0x9,
		R_RENDERTARGET_SHADOWCOOKIE_BLUR = 0xA,
		R_RENDERTARGET_POST_EFFECT_0 = 0xB,
		R_RENDERTARGET_POST_EFFECT_1 = 0xC,
		R_RENDERTARGET_SHADOWMAP_SUN = 0xD,
		R_RENDERTARGET_SHADOWMAP_SPOT = 0xE,
		R_RENDERTARGET_COUNT = 0xF,
		R_RENDERTARGET_NONE = 0x10,
	};

	struct GfxDepthOfField
	{
		float viewModelStart;
		float viewModelEnd;
		float nearStart;
		float nearEnd;
		float farStart;
		float farEnd;
		float nearBlur;
		float farBlur;
	};

	struct GfxFilm
	{
		bool enabled;
		float brightness;
		float contrast;
		float desaturation;
		bool invert;
		float tintDark[3];
		float tintLight[3];
	};

	struct GfxGlow
	{
		bool enabled;
		float bloomCutoff;
		float bloomDesaturation;
		float bloomIntensity;
		float radius;
	};

	struct GfxSunShadowBoundingPoly
	{
		float snapDelta[2];
		int pointCount;
		float points[9][2];
		int pointIsNear[9];
	};

	struct GfxSunShadowPartition
	{
		GfxViewParms shadowViewParms;
		int partitionIndex;
		GfxViewport viewport;
		ALIGN(1) GfxDrawSurfListInfo info;
		GfxSunShadowBoundingPoly boundingPoly;
	};

	struct GfxSunShadowProjection
	{
		float viewMatrix[4][4];
		float switchPartition[4];
		float shadowmapScale[4];
	};

	struct GfxSunShadow
	{
		GfxMatrix lookupMatrix;
		GfxSunShadowProjection sunProj;
		GfxSunShadowPartition partition[2];
	};

	struct GfxSpotShadow
	{
		GfxViewParms shadowViewParms;
		GfxMatrix lookupMatrix;
		char shadowableLightIndex;
		char pad[3];
		GfxLight* light;
		float fade;
		ALIGN(1) GfxDrawSurfListInfo info;
		GfxViewport viewport;
		GfxImage* image;
		GfxRenderTargetId renderTargetId;
		float pixelAdjust[4];
		int clearScreen;
		GfxMeshData* clearMesh;
	};

	struct GfxQuadMeshData
	{
		float x;
		float y;
		float width;
		float height;
		GfxMeshData meshData;
	};

	struct GfxDebugPoly
	{
		float color[4];
		int firstVert;
		int vertCount;
	};

	struct GfxCmdDrawText2D
	{
		GfxCmdHeader header;
		float x;
		float y;
		int rotation;
		Font_s* font;
		float xScale;
		float yScale;
		GfxColor color;
		int maxChars;
		int renderFlags;
		int cursorPos;
		char cursorLetter;
		float glowForceColor;
		int fxBirthTime;
		int fxLetterTime;
		int fxDecayStartTime;
		int fxDecayDuration;
		Material* fxMaterial;
		Material* fxMaterialGlow;
		int padding;
		const char* text;
	};

	struct trDebugString_t
	{
		float xyz[3];
		float color[4];
		float scale;
		char text[96];
	};

	struct trDebugLine_t
	{
		float start[3];
		float end[3];
		float color[4];
		int depthTest;
	};

	struct GfxDebugPlume
	{
		float origin[3];
		float color[4];
		int score;
		int startTime;
		int duration;
	};

	struct DebugGlobals
	{
		float (*verts)[3];
		int vertCount;
		int vertLimit;
		GfxDebugPoly* polys;
		int polyCount;
		int polyLimit;
		trDebugString_t* strings;
		int stringCount;
		int stringLimit;
		trDebugString_t* externStrings;
		int externStringCount;
		int externMaxStringCount;
		trDebugLine_t* lines;
		int lineCount;
		int lineLimit;
		trDebugLine_t* externLines;
		int externLineCount;
		int externMaxLineCount;
		GfxDebugPlume* plumes;
		int plumeCount;
		int plumeLimit;
	};

	struct clientDebugStringInfo_t
	{
		int max;
		int num;
		trDebugString_t* strings;
		int* durations;
	};

	struct alignas(16) GfxBackEndData
	{
		char surfsBuffer[131072];
		FxCodeMeshData codeMeshes[2048];
		uint32_t primDrawSurfsBuf[65536];
		GfxViewParms viewParms[28]; // GfxViewParms either has pad or zFar
		char primaryLightTechType[13][256];
		float codeMeshArgs[256][4];
		GfxParticleCloud clouds[256];
		GfxDrawSurf drawSurfs[32768];
		GfxMeshData codeMesh;
		GfxSModelCachedVertex smcPatchVerts[8192];
		uint16_t smcPatchList[256];
		uint32_t smcPatchCount;
		uint32_t smcPatchVertsUsed;
		GfxModelLightingPatch modelLightingPatchList[4096];
		volatile int modelLightingPatchCount;
		GfxBackEndPrimitiveData prim;
		uint32_t shadowableLightHasShadowMap[8];
		uint32_t frameCount;
		int drawSurfCount;
		volatile int surfPos;
		volatile int gfxEntCount;
		GfxEntity gfxEnts[128];
		volatile int cloudCount;
		volatile int codeMeshCount;
		volatile int codeMeshArgsCount;
		volatile int markMeshCount;
		FxMarkMeshData markMeshes[1536];
		GfxMeshData markMesh;
		GfxVertexBufferState* skinnedCacheVb;
		IDirect3DQuery9* endFence;
		char* tempSkinBuf;
		volatile int tempSkinPos;
		IDirect3DIndexBuffer9* preTessIb;
		int viewParmCount;
		GfxFog fogSettings;
		GfxCmdArray* commands;
		uint32_t viewInfoIndex;
		uint32_t viewInfoCount;
		GfxViewInfo* viewInfo;
		const void* cmds;
		GfxLight sunLight;
		int hasApproxSunDirChanged;
		volatile int primDrawSurfPos;
		uint32_t* staticModelLit;
		DebugGlobals debugGlobals;
		uint32_t drawType;
	};

	struct alignas(8) GfxCmdBufInput
	{
		float consts[58][4];
		GfxImage* codeImages[27];
		char codeImageSamplerStates[27];
		GfxBackEndData* data;
	};

	struct GfxViewInfo
	{
		GfxViewParms viewParms;
		GfxSceneDef sceneDef;
		GfxViewport sceneViewport;
		GfxViewport displayViewport;
		GfxViewport scissorViewport;
		ShadowType dynamicShadowType;
		alignas(16) ShadowCookieList shadowCookieList;
		int localClientNum;
		int isRenderingFullScreen;
		bool needsFloatZ;
		GfxLight shadowableLights[255];
		uint32_t shadowableLightCount;
		PointLightPartition pointLightPartitions[4];
		GfxMeshData pointLightMeshData[4];
		int pointLightCount;
		uint32_t emissiveSpotLightIndex;
		GfxLight emissiveSpotLight;
		int emissiveSpotDrawSurfCount;
		GfxDrawSurf* emissiveSpotDrawSurfs;
		uint32_t emissiveSpotLightCount;
		float blurRadius;
		float frustumPlanes[4][4];
		GfxDepthOfField dof;
		GfxFilm film;
		GfxGlow glow;
		const void* cmds;
		GfxSunShadow sunShadow;
		uint32_t spotShadowCount;
		GfxSpotShadow spotShadows[4];
		GfxQuadMeshData* fullSceneViewMesh;
		ALIGN(1) GfxDrawSurfListInfo litInfo;
		ALIGN(1) GfxDrawSurfListInfo decalInfo;
		ALIGN(1) GfxDrawSurfListInfo emissiveInfo;
		GfxCmdBufInput input;
	};

	struct GfxCodeMatrices
	{
		GfxMatrix matrix[32];
	};

	enum GfxViewMode
	{
		VIEW_MODE_NONE = 0x0,
		VIEW_MODE_3D = 0x1,
		VIEW_MODE_2D = 0x2,
		VIEW_MODE_IDENTITY = 0x3,
	};

	enum GfxViewportBehavior
	{
		GFX_USE_VIEWPORT_FOR_VIEW = 0x0,
		GFX_USE_VIEWPORT_FULL = 0x1,
	};

	enum CODEIMAGES
	{
		BlackSampler = 0x0,
		WhiteSampler = 0x1,
		IdentityNormalMapSampler = 0x2,
		Null3 = 0x3,
		Null4 = 0x4,
		Null5 = 0x5,
		ShadowCookieSampler = 0x6,
		ShadowmapSamplerSun = 0x7,
		ShadowmapSamplerSpot = 0x8,
		FeedbackSampler = 0x9,
		ResolvedPostSun = 0xA,
		ResolvedScene = 0xB,
		PostEffect0 = 0xC,
		PostEffect1 = 0xD,
		SkySampler = 0xE,
		AttenuationSampler = 0xF,
		DynamicShadowSampler = 0x10,
		OutdoorSampler = 0x11,
		FloatzSampler = 0x12,
		ProcessedFloatZSampler = 0x13,
		RawFloatZSampler = 0x14,
		CaseTextureSampler = 0x15,
		CinematicY = 0x16,
		CinematicCr = 0x17,
		CinematicCb = 0x18,
		CinematicA = 0x19,
		Null26 = 0x1A,
		Null27 = 0x1B,
	};

	struct alignas(16) GfxCmdBufSourceState
	{
		GfxCodeMatrices matrices;
		GfxCmdBufInput input;
		GfxViewParms viewParms;
		GfxMatrix shadowLookupMatrix;
		uint16_t constVersions[90];
		uint16_t matrixVersions[8];
		float eyeOffset[4];
		uint32_t shadowableLightForShadowLookupMatrix;
		GfxScaledPlacement* objectPlacement;
		GfxViewParms* viewParms3D;
		uint32_t depthHackFlags;
		GfxScaledPlacement skinnedPlacement;
		int cameraView;
		GfxViewMode viewMode;
		GfxSceneDef sceneDef;
		GfxViewport sceneViewport;
		float materialTime;
		GfxViewportBehavior viewportBehavior;
		int renderTargetWidth;
		int renderTargetHeight;
		bool viewportIsDirty;
		uint32_t shadowableLightIndex;
	};

	struct GfxImageFilterPass
	{
		Material* material;
		float srcWidth;
		float srcHeight;
		int dstWidth;
		int dstHeight;
		int tapHalfCount;
		float tapOffsetsAndWeights[8][4];
	};

	struct GfxImageFilter
	{
		int passCount;
		GfxImageFilterPass passes[32];
		GfxImage* sourceImage;
		GfxRenderTargetId finalTarget;
	};

	struct gfxVertexSteamsUnk
	{
		uint32_t stride;
		IDirect3DVertexBuffer9* vb;
		uint32_t offset;
	};

	enum GfxDepthRangeType
	{
		GFX_DEPTH_RANGE_SCENE = 0x0,
		GFX_DEPTH_RANGE_VIEWMODEL = 0x2,
		GFX_DEPTH_RANGE_FULL = 0xFFFFFFFF,
	};

	enum MaterialVertexDeclType
	{
		VERTDECL_GENERIC = 0x0,
		VERTDECL_PACKED = 0x1,
		VERTDECL_WORLD = 0x2,
		VERTDECL_WORLD_T1N0 = 0x3,
		VERTDECL_WORLD_T1N1 = 0x4,
		VERTDECL_WORLD_T2N0 = 0x5,
		VERTDECL_WORLD_T2N1 = 0x6,
		VERTDECL_WORLD_T2N2 = 0x7,
		VERTDECL_WORLD_T3N0 = 0x8,
		VERTDECL_WORLD_T3N1 = 0x9,
		VERTDECL_WORLD_T3N2 = 0xA,
		VERTDECL_WORLD_T4N0 = 0xB,
		VERTDECL_WORLD_T4N1 = 0xC,
		VERTDECL_WORLD_T4N2 = 0xD,
		VERTDECL_POS_TEX = 0xE,
		VERTDECL_STATICMODELCACHE = 0xF,
		VERTDECL_COUNT = 0x10,
	};

	struct GfxCmdBufPrimState
	{
		IDirect3DDevice9* device;
		IDirect3DIndexBuffer9* indexBuffer;
		MaterialVertexDeclType vertDeclType;
		gfxVertexSteamsUnk streams[2];
		IDirect3DVertexDeclaration9* vertexDecl;
	};

	struct GfxCmdBufState
	{
		char refSamplerState[16];
		uint32_t samplerState[16];
		GfxTexture* samplerTexture[16];
		GfxCmdBufPrimState prim;
		Material* material;
		MaterialTechniqueType techType;
		MaterialTechnique* technique;
		MaterialPass* pass;
		uint32_t passIndex;
		GfxDepthRangeType depthRangeType;
		float depthRangeNear;
		float depthRangeFar;
		uint64_t vertexShaderConstState[32];
		uint64_t pixelShaderConstState[256];
		char alphaRef;
		uint32_t refStateBits[2];
		uint32_t activeStateBits[2];
		MaterialPixelShader* pixelShader;
		MaterialVertexShader* vertexShader;
		GfxViewport viewport;
		GfxRenderTargetId renderTargetId;
		Material* origMaterial;
		MaterialTechniqueType origTechType;
	};

	struct GfxVertex
	{
		float xyzw[4];
		GfxColor color;
		float texCoord[2];
		PackedUnitVec normal;
	};

	struct GfxRenderTargetSurface
	{
		IDirect3DSurface9* color;
		IDirect3DSurface9* depthStencil;
	};

	struct GfxRenderTarget
	{
		GfxImage* image;
		GfxRenderTargetSurface surface;
		uint32_t width;
		uint32_t height;
	};

	struct materialCommands_t
	{
		GfxVertex verts[5450];
		uint16_t indices[1048576];
		MaterialVertexDeclType vertDeclType;
		uint32_t vertexSize;
		int indexCount;
		int vertexCount;
		int firstVertex;
		int lastVertex;
		bool finishedFilling;
		char pad[7];
	};

	struct clientLogo_t
	{
		int startTime;
		int duration;
		int fadein;
		int fadeout;
		Material* material[2];
	};

	struct serverInfo_t
	{
		netadr_t adrr;
		char allowAnonymous;
		char bPassword;
		char pure;
		char consoleDisabled;
		char netType;
		char clients;
		char maxClients;
		char dirty;
		char friendlyfire;
		char killcam;
		char hardware;
		char mod;
		char voice;
		char punkbuster;
		char requestCount;
		bool unk;
		short unk2;
		short minPing;
		short maxPing;
		short ping;
		char pad_0x002C[0x5];
		char hostName[32];
		char mapName[32];
		char game[24];
		char gameType[16];
		char pad_0x0099[0x3];
	};

	struct alignas(4) vidConfig_t
	{
		uint32_t sceneWidth;
		uint32_t sceneHeight;
		uint32_t displayWidth;
		uint32_t displayHeight;
		uint32_t displayFrequency;
		int isFullscreen;
		float aspectRatioWindow;
		float aspectRatioScenePixel;
		float aspectRatioDisplayPixel;
		uint32_t maxTextureSize;
		uint32_t maxTextureMaps;
		bool deviceSupportsGamma;
	};

	struct clientDebugLineInfo_t
	{
		int max;
		int num;
		trDebugLine_t* lines;
		int* durations;
	};

	struct clientDebug_t
	{
		int prevFromServer;
		int fromServer;
		clientDebugStringInfo_t clStrings;
		clientDebugStringInfo_t svStringsBuffer;
		clientDebugStringInfo_t svStrings;
		clientDebugLineInfo_t clLines;
		clientDebugLineInfo_t svLinesBuffer;
		clientDebugLineInfo_t svLines;
	};

	struct clientStatic_t
	{
		int quit;
		int hunkUsersStarted;
		char servername[256];
		int rendererStarted;
		int soundStarted;
		int uiStarted;
		int frametime;
		int realtime;
		int realFrametime;
		clientLogo_t logo;
		float mapCenter[3];
		int numlocalservers;
		serverInfo_t localServers[128];
		int ui_displayedServerAmount;
		int ui_totalServerAmount;
		int ui_someothercrap;
		int waitglobalserverresponse;
		int numglobalservers;
		serverInfo_t globalServers[20000];
		int numfavoriteservers;
		serverInfo_t favoriteServers[128];
		int pingUpdateSource;
		netadr_t updateServer;
		char updateChallenge[1024];
		char updateInfoString[1024];
		netadr_t authorizeServer;
		Material* whiteMaterial;
		Material* consoleMaterial;
		Font_s* consoleFont;
		char autoupdateServerNames[5][64];
		netadr_t autoupdateServer;
		vidConfig_t vidConfig;
		clientDebug_t debug;
		int download;
		char downloadTempName[256];
		char downloadName[256];
		int downloadNumber;
		int downloadBlock;
		int downloadCount;
		int downloadSize;
		char downloadList[1024];
		int downloadRestart;
		int gameDirChanged;
		int wwwDlDisconnected;
		int wwwDlInProgress;
		int downloadFlags;
		char originalDownloadName[64];
		float debugRenderPos[3];
	};

	struct fileInIwd_s
	{
		uint32_t pos;
		char* name;
		fileInIwd_s* next;
	};

	struct iwd_t
	{
		char iwdFilename[256];
		char iwdBasename[256];
		char iwdGamename[256];
		char* handle;
		int checksum;
		int pure_checksum;
		volatile int hasOpenFile;
		int numfiles;
		char referenced;
		uint32_t hashSize;
		fileInIwd_s** hashTable;
		fileInIwd_s* buildBuffer;
	};

	struct directory_t
	{
		char path[256];
		char gamedir[256];
	};

	struct searchpath_s
	{
		searchpath_s* next;
		iwd_t* iwd;
		directory_t* dir;
		int bLocalized;
		int ignore;
		int ignorePureCheck;
		int language;
	};

	struct rectDef_s
	{
		float x;
		float y;
		float w;
		float h;
		int horzAlign;
		int vertAlign;
	};

	struct windowDef_t
	{
		const char* name;
		rectDef_s rect;
		rectDef_s rectClient;
		const char* group;
		int style;
		int border;
		int ownerDraw;
		int ownerDrawFlags;
		float borderSize;
		int staticFlags;
		int dynamicFlags[1];
		int nextTime;
		float foreColor[4];
		float backColor[4];
		float borderColor[4];
		float outlineColor[4];
		Material* background;
	};

	union operandInternalDataUnion
	{
		int intVal;
		float floatVal;
		const char* string;
	};

	enum expDataType
	{
		VAL_INT = 0x0,
		VAL_FLOAT = 0x1,
		VAL_STRING = 0x2,
	};

	struct Operand
	{
		expDataType dataType;
		operandInternalDataUnion internals;
	};

	union entryInternalData
	{
		int operationEnum;
		Operand operand;
	};

	struct expressionEntry
	{
		int type;
		entryInternalData data;
	};

	struct statement_s
	{
		int numEntries;
		expressionEntry** entries;
	};

	struct ItemKeyHandler
	{
		int key;
		const char* action;
		ItemKeyHandler* next;
	};

	struct columnInfo_s
	{
		int pos;
		int width;
		int maxChars;
		int alignment;
	};

	struct listBoxDef_s
	{
		int mousePos;
		int startPos[1];
		int endPos[1];
		int drawPadding;
		float elementWidth;
		float elementHeight;
		int elementStyle;
		int numColumns;
		columnInfo_s columnInfo[16];
		const char* doubleClick;
		int notselectable;
		int noScrollBars;
		int usePaging;
		float selectBorder[4];
		float disableColor[4];
		Material* selectIcon;
	};

	struct editFieldDef_s
	{
		float minVal;
		float maxVal;
		float defVal;
		float range;
		int maxChars;
		int maxCharsGotoNext;
		int maxPaintChars;
		int paintOffset;
	};

	struct multiDef_s
	{
		const char* dvarList[32];
		const char* dvarStr[32];
		float dvarValue[32];
		int count;
		int strDef;
	};

	union itemDefData_t
	{
		listBoxDef_s* listBox;
		editFieldDef_s* editField;
		multiDef_s* multi;
		const char* enumDvarName;
		void* data;
	};

	struct menuDef_t
	{
		windowDef_t window;
		const char* font;
		int fullScreen;
		int itemCount;
		int fontIndex;
		int cursorItem[1];
		int fadeCycle;
		float fadeClamp;
		float fadeAmount;
		float fadeInAmount;
		float blurRadius;
		const char* onOpen;
		const char* onClose;
		const char* onESC;
		ItemKeyHandler* onKey;
		statement_s visibleExp;
		const char* allowedBinding;
		const char* soundName;
		int imageTrack;
		float focusColor[4];
		float disableColor[4];
		statement_s rectXExp;
		statement_s rectYExp;
		itemDef_s** items;
	};

	struct itemDef_s
	{
		windowDef_t window;
		rectDef_s textRect[1];
		int type;
		int dataType;
		int alignment;
		int fontEnum;
		int textAlignMode;
		float textalignx;
		float textaligny;
		float textscale;
		int textStyle;
		int gameMsgWindowIndex;
		int gameMsgWindowMode;
		const char* text;
		int itemFlags;
		menuDef_t* parent;
		const char* mouseEnterText;
		const char* mouseExitText;
		const char* mouseEnter;
		const char* mouseExit;
		const char* action;
		const char* onAccept;
		const char* onFocus;
		const char* leaveFocus;
		const char* dvar;
		const char* dvarTest;
		ItemKeyHandler* onKey;
		const char* enableDvar;
		int dvarFlags;
		snd_alias_list_t* focusSound;
		int feeder;
		int cursorPos[1];
		itemDefData_t typeData;
		int imageTrack;
		statement_s visibleExp;
		statement_s textExp;
		statement_s materialExp;
		statement_s rectXExp;
		statement_s rectYExp;
		statement_s rectWExp;
		statement_s rectHExp;
		statement_s forecolorAExp;
	};

	struct MenuList
	{
		const char* name;
		int menuCount;
		menuDef_t** menus;
	};

	struct cursor_t
	{
		float x;
		float y;
	};

	enum UILocalVarType
	{
		UILOCALVAR_INT = 0x0,
		UILOCALVAR_FLOAT = 0x1,
		UILOCALVAR_STRING = 0x2,
	};

	union $B42A88463653BDCDFC5664844B4491DA
	{
		int integer;
		float value;
		const char* string;
	};

	struct UILocalVar
	{
		UILocalVarType type;
		const char* name;
		$B42A88463653BDCDFC5664844B4491DA u;
	};

	struct UILocalVarContext
	{
		UILocalVar table[256];
	};

	struct UiContext
	{
		int localClientNum;
		float bias;
		int realTime;
		int frameTime;
		cursor_t cursor;
		int isCursorVisible;
		int screenWidth;
		int screenHeight;
		float screenAspect;
		float FPS;
		float blurRadiusOut;
		menuDef_t* Menus[640];
		int menuCount;
		menuDef_t* menuStack[16];
		int openMenuCount;
		UILocalVarContext localVars;
	};

	enum LocSelInputState
	{
		LOC_SEL_INPUT_NONE = 0x0,
		LOC_SEL_INPUT_CONFIRM = 0x1,
		LOC_SEL_INPUT_CANCEL = 0x2,
	};

	struct KeyState
	{
		int down;
		int repeats;
		const char* binding;
	};

	struct PlayerKeyState
	{
		int chat_team;
		int overstrikeMode;
		int anyKeyDown;
		KeyState keys[256];
		LocSelInputState locSelInputState;
	};

	enum connstate_t
	{
		CA_DISCONNECTED = 0x0,
		CA_CINEMATIC = 0x1,
		CA_LOGO = 0x2,
		CA_CONNECTING = 0x3,
		CA_CHALLENGING = 0x4,
		CA_CONNECTED = 0x5,
		CA_SENDINGSTATS = 0x6,
		CA_LOADING = 0x7,
		CA_PRIMED = 0x8,
		CA_ACTIVE = 0x9,
	};

	struct clientUIActive_t
	{
		bool active;
		bool isRunning;
		bool cgameInitialized;
		bool cgameInitCalled;
		int keyCatchers;
		bool displayHUDWithKeycatchUI;
		connstate_t connectionState;
	};

	struct score_t
	{
		int client;
		int score;
		int ping;
		int deaths;
		int team;
		int kills;
		int rank;
		int assists;
		Material* hStatusIcon;
		Material* hRankIcon;
	};

	struct refdef_s
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
		float tanHalfFovX;
		float tanHalfFovY;
		vec3 vieworg;
		mat3 viewaxis;
		vec3 viewOffset;
		int time;
		float zNear;
		float blurRadius;
		GfxDepthOfField dof;
		GfxFilm film;
		GfxGlow glow;
		GfxLight primaryLights[255];
		GfxViewport scissorViewport;
		bool useScissorViewport;
		int localClientNum;
	};

	struct playerEntity_t
	{
		float fLastWeaponPosFrac;
		int bPositionToADS;
		float vPositionLastOrg[3];
		float fLastIdleFactor;
		float vLastMoveOrg[3];
		float vLastMoveAng[3];
	};

	struct GfxSkinCacheEntry
	{
		uint32_t frameCount;
		int skinnedCachedOffset;
		uint16_t numSkinnedVerts;
		uint16_t ageCount;
	};

	struct cpose_t
	{
		uint16_t lightingHandle;
		char eType;
		char eTypeUnion;
		char localClientNum;
		int cullIn;
		char isRagdoll;
		int ragdollHandle;
		int killcamRagdollHandle;
		int physObjId;
		vec3 origin;
		vec3 angles;
		GfxSkinCacheEntry skinCacheEntry;
		char pad[0x24];
	};

	struct centity_s
	{
		cpose_t pose;
		LerpEntityState currentState;
		entityState_s nextState;
		bool isAlive;
		bool bMuzzleFlash;
		bool bTrailMade;
		int previousEventSequence;
		int miscTime;
		vec3 lightingOrigin;
		XAnimTree_s* tree;
	};

	enum InvalidCmdHintType
	{
		INVALID_CMD_NONE = 0x0,
		INVALID_CMD_NO_AMMO_BULLETS = 0x1,
		INVALID_CMD_NO_AMMO_FRAG_GRENADE = 0x2,
		INVALID_CMD_NO_AMMO_SPECIAL_GRENADE = 0x3,
		INVALID_CMD_NO_AMMO_FLASH_GRENADE = 0x4,
		INVALID_CMD_STAND_BLOCKED = 0x5,
		INVALID_CMD_CROUCH_BLOCKED = 0x6,
		INVALID_CMD_TARGET_TOO_CLOSE = 0x7,
		INVALID_CMD_LOCKON_REQUIRED = 0x8,
		INVALID_CMD_NOT_ENOUGH_CLEARANCE = 0x9,
	};

	struct $F6DFD6D87F75480A1EF1906639406DF5
	{
		int time;
		int duration;
	};

	struct shellshock_t
	{
		void* parms; // shellshock_parms_t
		int startTime;
		int duration;
		int loopEndTime;
		float sensitivity;
		float viewDelta[2];
		int hasSavedScreen;
	};

	struct viewDamage_t
	{
		int time;
		int duration;
		float yaw;
	};

	struct animScriptCommand_t
	{
		short bodyPart[2];
		short animIndex[2];
		short animDuration[2];
		snd_alias_list_t* soundAlias;
	};

	struct animScriptCondition_t
	{
		int index;
		uint32_t value[2];
	};

	struct animScriptItem_t
	{
		int numConditions;
		animScriptCondition_t conditions[10];
		int numCommands;
		animScriptCommand_t commands[8];
	};

	struct animScript_t
	{
		int numItems;
		animScriptItem_t* items[128];
	};

	struct alignas(8) animation_s
	{
		char name[64];
		int initialLerp;
		float moveSpeed;
		int duration;
		int nameHash;
		int flags;
		int64_t movetype;
		int noteType;
	};

	struct scr_animtree_t
	{
		struct XAnim_s* anims;
	};

	struct alignas(8) animScriptData_t
	{
		animation_s animations[512];
		uint32_t numAnimations;
		animScript_t scriptAnims[1][43];
		animScript_t scriptCannedAnims[1][43];
		animScript_t scriptStateChange[1][1];
		animScript_t scriptEvents[21];
		animScriptItem_t scriptItems[2048];
		int numScriptItems;
		scr_animtree_t animTree;
		uint16_t torsoAnim;
		uint16_t legsAnim;
		uint16_t turningAnim;
		snd_alias_list_t*(__cdecl* soundAlias)(const char*);
		int(__cdecl* playSoundAlias)(int, snd_alias_list_t*);
	};

	struct $0867E0FC4F8157A276DAB76B1612E229
	{
		scr_animtree_t tree;
		int torso;	 // scr_anim_s
		int legs;	 // scr_anim_s
		int turning; // scr_anim_s
	};

	struct lerpFrame_t
	{
		float yawAngle;
		int yawing;
		float pitchAngle;
		int pitching;
		int animationNumber;
		void* animation;
		int animationTime;
		float oldFramePos[3];
		float animSpeedScale;
		int oldFrameSnapshotTime;
	};

	struct clientControllers_t
	{
		float angles[6][3];
		float tag_origin_angles[3];
		float tag_origin_offset[3];
	};

	struct alignas(4) clientInfo_t
	{
		int infoValid;
		int isAlive;
		int clientNum;
		char name[16];
		team_t team;
		team_t oldteam;
		int rank;
		int prestige;
		int perks;
		int score;
		int location;
		int health;
		char model[64];
		char attachModelNames[6][64];
		char attachTagNames[6][64];
		lerpFrame_t legs;
		lerpFrame_t torso;
		float lerpMoveDir;
		float lerpLean;
		float playerAngles[3];
		int leftHandGun;
		int dobjDirty;
		clientControllers_t control;
		uint32_t clientConditions[10][2];
		XAnimTree_s* pXAnimTree;
		int iDObjWeapon;
		char weaponModel;
		int stanceTransitionTime;
		int turnAnimEndTime;
		char turnAnimType;
		int attachedVehEntNum;
		int attachedVehSlotIndex;
		bool hideWeapon;
		bool usingKnife;
	};

	struct bgs_t
	{
		animScriptData_t animScriptData;
		$0867E0FC4F8157A276DAB76B1612E229 generic_human;
		int time;
		int latestSnapshotTime;
		int frametime;
		int anim_user;
		XModel*(__cdecl* GetXModel)(const char*);
		void* CreateDObj;
		uint16_t AttachWeapon;
		void* DObj;
		void(__cdecl* SafeDObjFree)(int, int);
		void*(__cdecl* AllocXAnim)(int);
		clientInfo_t clientinfo[64];
	};

	struct visionSetVars_t
	{
		bool glowEnable;
		float glowBloomCutoff;
		float glowBloomDesaturation;
		float glowBloomIntensity0;
		float glowBloomIntensity1;
		float glowRadius0;
		float glowRadius1;
		float glowSkyBleedIntensity0;
		float glowSkyBleedIntensity1;
		bool filmEnable;
		float filmBrightness;
		float filmContrast;
		float filmDesaturation;
		bool filmInvert;
		float filmLightTint[3];
		float filmDarkTint[3];
	};

	enum visionSetLerpStyle_t
	{
		VISIONSETLERP_UNDEFINED = 0x0,
		VISIONSETLERP_NONE = 0x1,
		VISIONSETLERP_TO_LINEAR = 0x2,
		VISIONSETLERP_TO_SMOOTH = 0x3,
		VISIONSETLERP_BACKFORTH_LINEAR = 0x4,
		VISIONSETLERP_BACKFORTH_SMOOTH = 0x5,
	};

	struct visionSetLerpData_t
	{
		int timeStart;
		int timeDuration;
		visionSetLerpStyle_t style;
	};

	struct $BE9F66374A020A9809EEAF403416A176
	{
		float aimSpreadScale;
	};

	struct hudElemSoundInfo_t
	{
		int lastPlayedTime;
	};

	struct cg_s
	{
		int clientNum;
		int localClientNum;
		DemoType demoType;
		CubemapShot cubemapShot;
		int cubemapSize;
		int renderScreen;
		int latestSnapshotNum;
		int latestSnapshotTime;
		snapshot_s* snap;
		snapshot_s* nextSnap;
		snapshot_s activeSnapshots[2];
		float frameInterpolation;
		int frametime;
		int time;
		int oldTime;
		int physicsTime;
		int mapRestart;
		int renderingThirdPerson;
		playerState_s predictedPlayerState;
		centity_s predictedPlayerEntity;
		playerEntity_t playerEntity;
		int predictedErrorTime;
		vec3 predictedError;
		float landChange;
		int landTime;
		float heightToCeiling;
		refdef_s refdef;
		vec3 refdefViewAngles;
		vec3 lastVieworg;
		vec3 swayViewAngles;
		vec3 swayAngles;
		vec3 swayOffset;
		int iEntityLastType[1024];
		XModel* pEntityLastXModel[1024];
		float zoomSensitivity;
		bool isLoading;
		char objectiveText[1024];
		char scriptMainMenu[256];
		int scoresRequestTime;
		int numScores;
		int teamScores[4];
		int teamPings[4];
		int teamPlayers[4];
		score_t scores[64];
		int scoreLimit;
		int showScores;
		int scoreFadeTime;
		int scoresTop;
		int scoresOffBottom;
		int scoresBottom;
		int drawHud;
		int crosshairClientNum;
		int crosshairClientLastTime;
		int crosshairClientStartTime;
		int identifyClientNum;
		int cursorHintIcon;
		int cursorHintTime;
		int cursorHintFade;
		int cursorHintString;
		int lastClipFlashTime;
		InvalidCmdHintType invalidCmdHintType;
		int invalidCmdHintTime;
		int lastHealthPulseTime;
		int lastHealthLerpDelay;
		int lastHealthClient;
		float lastHealth;
		float healthOverlayFromAlpha;
		float healthOverlayToAlpha;
		int healthOverlayPulseTime;
		int healthOverlayPulseDuration;
		int healthOverlayPulsePhase;
		bool healthOverlayHurt;
		int healthOverlayLastHitTime;
		float healthOverlayOldHealth;
		int healthOverlayPulseIndex;
		int proneBlockedEndTime;
		int lastStance;
		int lastStanceChangeTime;
		int lastStanceFlashTime;
		int voiceTime;
		uint32_t weaponSelect;
		int weaponSelectTime;
		uint32_t weaponLatestPrimaryIdx;
		int prevViewmodelWeapon;
		int equippedOffHand;
		viewDamage_t viewDamage[8];
		int damageTime;
		float damageX;
		float damageY;
		float damageValue;
		float viewFade;
		int weapIdleTime;
		int nomarks;
		int v_dmg_time;
		float v_dmg_pitch;
		float v_dmg_roll;
		float fBobCycle;
		float xyspeed;
		vec3 kickAVel;
		vec3 kickAngles;
		vec3 offsetAngles;
		float gunPitch;
		float gunYaw;
		float gunXOfs;
		float gunYOfs;
		float gunZOfs;
		vec3 vGunOffset;
		vec3 vGunSpeed;
		float viewModelAxis[4][3];
		float rumbleScale;
		float compassNorthYaw;
		float compassNorth[2];
		Material* compassMapMaterial;
		float compassMapUpperLeft[2];
		float compassMapWorldSize[2];
		int compassFadeTime;
		int healthFadeTime;
		int ammoFadeTime;
		int stanceFadeTime;
		int sprintFadeTime;
		int offhandFadeTime;
		int offhandFlashTime;
		shellshock_t shellshock;
		$F6DFD6D87F75480A1EF1906639406DF5 testShock;
		int holdBreathTime;
		int holdBreathInTime;
		int holdBreathDelay;
		float holdBreathFrac;
		float radarProgress;
		float selectedLocation[2];
		SprintState sprintStates;
		int _unk01;
		int _unk02;
		bgs_t bgs;
		cpose_t viewModelPose;
		visionSetVars_t visionSetPreLoaded[4];
		char visionSetPreLoadedName[4][64];
		visionSetVars_t visionSetFrom[2];
		visionSetVars_t visionSetTo[2];
		visionSetVars_t visionSetCurrent[2];
		visionSetLerpData_t visionSetLerpData[2];
		char visionNameNaked[64];
		char visionNameNight[64];
		int extraButtons;
		int lastActionSlotTime;
		bool playerTeleported;
		int stepViewStart;
		float stepViewChange;
		$BE9F66374A020A9809EEAF403416A176 lastFrame;
		hudElemSoundInfo_t hudElemSound[32];
		int vehicleFrame;
	};

	struct trigger_info_t
	{
		uint16_t entnum;
		uint16_t otherEntnum;
		int useCount;
		int otherUseCount;
	};

	struct com_parse_mark_t
	{
		int lines;
		const char* text;
		int ungetToken;
		int backup_lines;
		const char* backup_text;
	};

	struct cached_tag_mat_t
	{
		int time;
		int entnum;
		uint16_t name;
		float tagMat[4][3];
	};

	struct level_locals_t
	{
		gclient_s* clients;
		gentity_s* gentities;
		int gentitySize;
		int num_entities;
		gentity_s* firstFreeEnt;
		gentity_s* lastFreeEnt;
		int logFile;
		int initializing;
		int clientIsSpawning;
		objective_t objectives[16];
		int maxclients;
		int framenum;
		int time;
		int previousTime;
		int frametime;
		int startTime;
		int teamScores[4];
		int lastTeammateHealthTime;
		int bUpdateScoresForIntermission;
		bool teamHasRadar[4];
		int manualNameChange;
		int numConnectedClients;
		int sortedClients[64];
		char voteString[1024];
		char voteDisplayString[1024];
		int voteTime;
		int voteExecuteTime;
		int voteYes;
		int voteNo;
		int numVotingClients;
		SpawnVar spawnVar;
		int savepersist;
		EntHandle droppedWeaponCue[32];
		float fFogOpaqueDist;
		float fFogOpaqueDistSqrd;
		int remapCount;
		int currentPlayerClone;
		trigger_info_t pendingTriggerList[256];
		trigger_info_t currentTriggerList[256];
		int pendingTriggerListSize;
		int currentTriggerListSize;
		int finished;
		int bPlayerIgnoreRadiusDamage;
		int bPlayerIgnoreRadiusDamageLatched;
		int registerWeapons;
		int bRegisterItems;
		int currentEntityThink;
		int openScriptIOFileHandles[1];
		char* openScriptIOFileBuffers[1];
		com_parse_mark_t currentScriptIOLineMark[1];
		cached_tag_mat_t cachedTagMat;
		int scriptPrintChannel;
		float compassMapUpperLeft[2];
		float compassMapWorldSize[2];
		float compassNorth[2];
		scr_vehicle_s* vehicles;
	};

	struct GfxCachedShaderText
	{
		const char* name;
		const char* text;
		int textSize;
	};

	struct MaterialString
	{
		const char* string;
		uint32_t hash;
	};

	struct MaterialStateMapRule
	{
		uint32_t stateBitsMask[2];
		uint32_t stateBitsValue[2];
		uint32_t stateBitsSet[2];
		uint32_t stateBitsClear[2];
	};

	struct MaterialStateMapRuleSet
	{
		int ruleCount;
		MaterialStateMapRule rules[1];
	};

	struct MaterialStateMap
	{
		const char* name;
		MaterialStateMapRuleSet* ruleSet[10];
	};

	struct MaterialInfoRaw
	{
		uint32_t nameOffset;
		uint32_t refImageNameOffset;
		char gameFlags;
		char sortKey;
		char textureAtlasRowCount;
		char textureAtlasColumnCount;
		float maxDeformMove;
		char deformFlags;
		char usage;
		uint16_t toolFlags;
		uint32_t locale;
		uint16_t autoTexScaleWidth;
		uint16_t autoTexScaleHeight;
		float tessSize;
		int surfaceFlags;
		int contents;
	};

	struct MaterialRaw
	{
		MaterialInfoRaw info;
		uint32_t refStateBits[2];
		uint16_t textureCount;
		uint16_t constantCount;
		uint32_t techSetNameOffset;
		uint32_t textureTableOffset;
		uint32_t constantTableOffset;
	};

	struct MaterialLoadGlob
	{
		uint32_t cachedShaderCount;
		GfxCachedShaderText* cachedShaderText;
		uint32_t vertexDeclCount;
		MaterialVertexDeclaration vertexDeclHashTable[32];
		uint32_t literalCount;
		float literalTable[16][4];
		uint32_t stringCount;
		MaterialString stringHashTable[64];
		uint32_t vertexShaderCount;
		MaterialVertexShader* vertexShaderHashTable[2][2048];
		uint32_t pixelShaderCount;
		MaterialPixelShader* pixelShaderHashTable[2][2048];
		uint32_t stateMapCount;
		MaterialStateMap* stateMapHashTable[32];
		uint32_t techniqueCount;
		MaterialTechnique* techniqueHashTable[2][4096];
		MaterialRaw* sortMtlRaw;
	};

	struct GfxDynamicIndices
	{
		volatile int used;
		int total;
		uint16_t* indices;
	};

	struct GfxIndexBufferState
	{
		volatile int used;
		int total;
		IDirect3DIndexBuffer9* buffer;
		uint16_t* indices;
	};

	struct GfxPackedVertexNormal
	{
		PackedUnitVec normal;
		PackedUnitVec tangent;
	};

	struct alignas(4) GfxBuffers
	{
		GfxDynamicIndices smodelCache;
		IDirect3DVertexBuffer9* smodelCacheVb;
		GfxIndexBufferState preTessIndexBufferPool[2];
		GfxIndexBufferState* preTessIndexBuffer;
		int preTessBufferFrame;
		GfxIndexBufferState dynamicIndexBufferPool[1];
		GfxIndexBufferState* dynamicIndexBuffer;
		GfxVertexBufferState skinnedCacheVbPool[2];
		char* skinnedCacheLockAddr;
		GfxVertexBufferState dynamicVertexBufferPool[1];
		GfxVertexBufferState* dynamicVertexBuffer;
		IDirect3DVertexBuffer9* particleCloudVertexBuffer;
		IDirect3DIndexBuffer9* particleCloudIndexBuffer;
		int dynamicBufferFrame;
		GfxPackedVertexNormal skinnedCacheNormals[2][147456];
		GfxPackedVertexNormal* skinnedCacheNormalsAddr;
		GfxPackedVertexNormal* oldSkinnedCacheNormalsAddr;
		uint32_t skinnedCacheNormalsFrameCount;
		bool fastSkin;
		bool skinCache;
	};

	struct GfxVisibleLight
	{
		int drawSurfCount;
		GfxDrawSurf drawSurfs[1024];
	};

	struct GfxShadowCookie
	{
		DpvsPlane planes[5];
		volatile int drawSurfCount;
		GfxDrawSurf drawSurfs[256];
	};

	struct GfxSkinnedXModelSurfs
	{
		void* firstSurf;
	};

	struct GfxSceneEntityCull
	{
		volatile uint32_t state;
		float mins[3];
		float maxs[3];
		char lods[32];
		GfxSkinnedXModelSurfs skinnedSurfs;
	};

	union GfxSceneEntityInfo
	{
		cpose_t* pose;
		uint16_t* cachedLightingHandle;
	};

	struct XAnimParent
	{
		uint16_t flags;
		uint16_t children;
	};

	union $2714E77E76DE9429E851020801EAFDE5
	{
		XAnimParts* parts;
		XAnimParent animParent;
	};

	struct XAnimEntry
	{
		uint16_t numAnims;
		uint16_t parent;
		$2714E77E76DE9429E851020801EAFDE5 ___u2;
	};

	struct XAnim_s
	{
		const char* debugName;
		uint32_t size;
		const char** debugAnimNames;
		XAnimEntry entries[1];
	};

	struct alignas(4) XAnimTree_s
	{
		XAnim_s* anims;
		int info_usage;
		volatile int calcRefCount;
		volatile int modifyRefCount;
		uint16_t children;
	};

	struct DSkelPartBits
	{
		int anim[4];
		int control[4];
		int skel[4];
	};

	struct DSkel
	{
		DSkelPartBits partBits;
		int timeStamp;
		DObjAnimMat* mat;
	};

	struct DObj_s
	{
		XAnimTree_s* tree;
		uint16_t duplicateParts;
		uint16_t entnum;
		char duplicatePartsSize;
		char numModels;
		char numBones;
		uint32_t ignoreCollision;
		volatile int locked;
		DSkel skel;
		float radius;
		uint32_t hidePartBits[4];
		XModel** models;
	};

	struct alignas(4) GfxSceneEntity
	{
		float lightingOrigin[3];
		GfxScaledPlacement placement;
		GfxSceneEntityCull cull;
		uint16_t gfxEntIndex;
		uint16_t entnum;
		DObj_s* obj;
		GfxSceneEntityInfo info;
		char reflectionProbeIndex;
	};

	struct alignas(4) GfxSceneModel
	{
		XModelDrawInfo info;
		XModel* model;
		DObj_s* obj;
		GfxScaledPlacement placement;
		uint16_t gfxEntIndex;
		uint16_t entnum;
		float radius;
		uint16_t* cachedLightingHandle;
		float lightingOrigin[3];
		char reflectionProbeIndex;
	};

	struct alignas(4) GfxSceneBrush
	{
		BModelDrawInfo info;
		uint16_t entnum;
		GfxBrushModel* bmodel;
		GfxPlacement placement;
		char reflectionProbeIndex;
	};

	union GfxEntCellRefInfo
	{
		float radius;
		GfxBrushModel* bmodel;
	};

	struct GfxSceneDpvs
	{
		uint32_t localClientNum;
		char* entVisData[7];
		uint16_t* sceneXModelIndex;
		uint16_t* sceneDObjIndex;
		GfxEntCellRefInfo* entInfo[4];
	};

	struct alignas(64) GfxScene
	{
		GfxDrawSurf bspDrawSurfs[8192];
		GfxDrawSurf smodelDrawSurfsLight[8192];
		GfxDrawSurf entDrawSurfsLight[8192];
		GfxDrawSurf bspDrawSurfsDecal[512];
		GfxDrawSurf smodelDrawSurfsDecal[512];
		GfxDrawSurf entDrawSurfsDecal[512];
		GfxDrawSurf bspDrawSurfsEmissive[8192];
		GfxDrawSurf smodelDrawSurfsEmissive[8192];
		GfxDrawSurf entDrawSurfsEmissive[8192];
		GfxDrawSurf fxDrawSurfsEmissive[8192];
		GfxDrawSurf fxDrawSurfsEmissiveAuto[8192];
		GfxDrawSurf fxDrawSurfsEmissiveDecal[8192];
		GfxDrawSurf bspSunShadowDrawSurfs0[4096];
		GfxDrawSurf smodelSunShadowDrawSurfs0[4096];
		GfxDrawSurf entSunShadowDrawSurfs0[4096];
		GfxDrawSurf bspSunShadowDrawSurfs1[8192];
		GfxDrawSurf smodelSunShadowDrawSurfs1[8192];
		GfxDrawSurf entSunShadowDrawSurfs1[8192];
		GfxDrawSurf bspSpotShadowDrawSurfs0[256];
		GfxDrawSurf smodelSpotShadowDrawSurfs0[256];
		GfxDrawSurf entSpotShadowDrawSurfs0[512];
		GfxDrawSurf bspSpotShadowDrawSurfs1[256];
		GfxDrawSurf smodelSpotShadowDrawSurfs1[256];
		GfxDrawSurf entSpotShadowDrawSurfs1[512];
		GfxDrawSurf bspSpotShadowDrawSurfs2[256];
		GfxDrawSurf smodelSpotShadowDrawSurfs2[256];
		GfxDrawSurf entSpotShadowDrawSurfs2[512];
		GfxDrawSurf bspSpotShadowDrawSurfs3[256];
		GfxDrawSurf smodelSpotShadowDrawSurfs3[256];
		GfxDrawSurf entSpotShadowDrawSurfs3[512];
		GfxDrawSurf shadowDrawSurfs[512];
		uint32_t shadowableLightIsUsed[32];
		int maxDrawSurfCount[34];
		volatile int drawSurfCount[34];
		GfxDrawSurf* drawSurfs[34];
		GfxDrawSurf fxDrawSurfsLight[8192];
		GfxDrawSurf fxDrawSurfsLightAuto[8192];
		GfxDrawSurf fxDrawSurfsLightDecal[8192];
		GfxSceneDef def;
		int addedLightCount;
		GfxLight addedLight[32];
		bool isAddedLightCulled[32];
		float dynamicSpotLightNearPlaneOffset;
		GfxVisibleLight visLight[4];
		GfxVisibleLight visLightShadow[1];
		GfxShadowCookie cookie[24];
		uint32_t* entOverflowedDrawBuf;
		volatile int sceneDObjCount;
		GfxSceneEntity sceneDObj[512];
		char sceneDObjVisData[7][512];
		volatile int sceneModelCount;
		GfxSceneModel sceneModel[1024];
		char sceneModelVisData[7][1024];
		volatile int sceneBrushCount;
		GfxSceneBrush sceneBrush[512];
		char sceneBrushVisData[3][512];
		uint32_t sceneDynModelCount;
		uint32_t sceneDynBrushCount;
		DpvsPlane shadowFarPlane[2];
		DpvsPlane shadowNearPlane[2];
		GfxSceneDpvs dpvs;
	};

	struct clSnapshot_t
	{
		int valid;
		int snapFlags;
		int serverTime;
		int messageNum;
		int deltaNum;
		int ping;
		int cmdNum;
		playerState_s ps;
		int numEntities;
		int numClients;
		int parseEntitiesNum;
		int parseClientsNum;
		int serverCommandNum;
	};

	struct gameState_t
	{
		int stringOffsets[2442];
		char stringData[131072];
		int dataCount;
	};

	enum StanceState
	{
		CL_STANCE_STAND = 0x0,
		CL_STANCE_CROUCH = 0x1,
		CL_STANCE_PRONE = 0x2,
	};

	struct ClientArchiveData
	{
		int serverTime;
		float origin[3];
		float velocity[3];
		int bobCycle;
		int movementDir;
		float viewangles[3];
	};

	struct outPacket_t
	{
		int p_cmdNumber;
		int p_serverTime;
		int p_realtime;
	};

	struct clientActive_t
	{
		bool usingAds;
		int timeoutcount;
		clSnapshot_t snap;
		bool alwaysFalse;
		int serverTime;
		int oldServerTime;
		int oldFrameServerTime;
		int serverTimeDelta;
		int oldSnapServerTime;
		int extrapolatedSnapshot;
		int newSnapshots;
		gameState_t gameState;
		char mapname[64];
		int parseEntitiesNum;
		int parseClientsNum;
		int mouseDx[2];
		int mouseDy[2];
		int mouseIndex;
		bool stanceHeld;
		StanceState stance;
		StanceState stancePosition;
		int stanceTime;
		int cgameUserCmdWeapon;
		int cgameUserCmdOffHandIndex;
		float cgameFOVSensitivityScale;
		float cgameMaxPitchSpeed;
		float cgameMaxYawSpeed;
		vec3 cgameKickAngles;
		vec3 cgameOrigin;
		vec3 cgameVelocity;
		vec3 cgameViewangles;
		int cgameBobCycle;
		int cgameMovementDir;
		int cgameExtraButtons;
		int cgamePredictedDataServerTime;
		vec3 viewangles;
		int serverId;
		int skelTimeStamp;
		volatile int skelMemPos;
		char skelMemory[262144];
		char* skelMemoryStart;
		bool allowedAllocSkel;
		alignas(4) usercmd_s cmds[128];
		int cmdNumber;
		ClientArchiveData clientArchive[256];
		int clientArchiveIndex;
		outPacket_t outPackets[32];
		clSnapshot_t snapshots[32];
		entityState_s entityBaselines[1024];
		entityState_s parseEntities[2048];
		clientState_s parseClients[2048];
		int corruptedTranslationFile;
		char translationVersion[256];
		float vehicleViewYaw;
		float vehicleViewPitch;
	};

	struct alignas(128) r_global_permanent_t
	{
		Material* sortedMaterials[2048];
		int needSortMaterials;
		int materialCount;
		GfxImage* whiteImage;
		GfxImage* blackImage;
		GfxImage* blackImage3D;
		GfxImage* blackImageCube;
		GfxImage* grayImage;
		GfxImage* identityNormalMapImage;
		GfxImage* specularityImage;
		GfxImage* outdoorImage;
		GfxImage* pixelCostColorCodeImage;
		GfxLightDef* dlightDef;
		Material* defaultMaterial;
		Material* whiteMaterial;
		Material* additiveMaterial;
		Material* pointMaterial;
		Material* lineMaterial;
		Material* lineMaterialNoDepth;
		Material* clearAlphaStencilMaterial;
		Material* shadowClearMaterial;
		Material* shadowCookieOverlayMaterial;
		Material* shadowCookieBlurMaterial;
		Material* shadowCasterMaterial;
		Material* shadowOverlayMaterial;
		Material* depthPrepassMaterial;
		Material* glareBlindMaterial;
		Material* pixelCostAddDepthAlwaysMaterial;
		Material* pixelCostAddDepthDisableMaterial;
		Material* pixelCostAddDepthEqualMaterial;
		Material* pixelCostAddDepthLessMaterial;
		Material* pixelCostAddDepthWriteMaterial;
		Material* pixelCostAddNoDepthWriteMaterial;
		Material* pixelCostColorCodeMaterial;
		Material* stencilShadowMaterial;
		Material* stencilDisplayMaterial;
		Material* floatZDisplayMaterial;
		Material* colorChannelMixerMaterial;
		Material* frameColorDebugMaterial;
		Material* frameAlphaDebugMaterial;
		GfxImage* rawImage;
		GfxWorld* world;
		Material* feedbackReplaceMaterial;
		Material* feedbackBlendMaterial;
		Material* feedbackFilmBlendMaterial;
		Material* cinematicMaterial;
		Material* dofDownsampleMaterial;
		Material* dofNearCocMaterial;
		Material* smallBlurMaterial;
		Material* postFxDofMaterial;
		Material* postFxDofColorMaterial;
		Material* postFxColorMaterial;
		Material* postFxMaterial;
		Material* symmetricFilterMaterial[8];
		Material* shellShockBlurredMaterial;
		Material* shellShockFlashedMaterial;
		Material* glowConsistentSetupMaterial;
		Material* glowApplyBloomMaterial;
		int savedScreenTimes[4];
	};

	union XAssetHeader
	{
		void* data;
		//XModelPieces* xmodelPieces;
		PhysPreset* physPreset;
		XAnimParts* parts;
		XModel* model;
		Material* material;
		MaterialPixelShader* pixelShader;
		MaterialVertexShader* vertexShader;
		MaterialTechniqueSet* techniqueSet;
		GfxImage* image;
		snd_alias_list_t* sound;
		//SndCurve* sndCurve;
		clipMap_t* clipMap;
		ComWorld* comWorld;
		GameWorldSp* gameWorldSp;
		GameWorldMp* gameWorldMp;
		MapEnts* mapEnts;
		GfxWorld* gfxWorld;
		GfxLightDef* lightDef;
		Font_s* font;
		MenuList* menuList;
		menuDef_t* menu;
		//LocalizeEntry* localize;
		WeaponDef* weapon;
		//SndDriverGlobals* sndDriverGlobals;
		FxEffectDef* fx;
		//FxImpactTable* impactFx;
		RawFile* rawfile;
		//StringTable* stringTable;
	};

	struct XAsset
	{
		XAssetType type;
		XAssetHeader header;
	};

	struct XAssetEntry
	{
		XAsset asset;
		char zoneIndex;
		char inuse;
		uint16_t nextHash;
		uint16_t nextOverride;
		uint16_t usageFrame;
	};

	struct XBlock
	{
		char* data;
		uint32_t size;
	};

	struct XZoneMemory
	{
		XBlock blocks[9];
		char* lockedVertexData;
		char* lockedIndexData;
		void* vertexBuffer;
		void* indexBuffer;
	};

	struct XZone
	{
		char name[64];
		int flags;
		int allocType;
		XZoneMemory mem;
		int fileSize;
		char modZone;
	};

	struct Image_MemUsage
	{
		int total;
		int lightmap;
		int minspec;
	};

	struct trStatistics_t
	{
		int c_indexes;
		int c_fxIndexes;
		int c_viewIndexes;
		int c_shadowIndexes;
		int c_vertexes;
		int c_batches;
		float dc;
		Image_MemUsage c_imageUsage;
	};

	struct GfxLodRamp
	{
		float scale;
		float bias;
	};

	struct alignas(4) GfxLodParms
	{
		float origin[3];
		GfxLodRamp ramp[2];
	};

	struct alignas(8) r_globals_t
	{
		GfxViewParms identityViewParms;
		bool inFrame;
		bool registered;
		bool forbidDelayLoadImages;
		bool ignorePrecacheErrors;
		float viewOrg[3];
		float viewDir[3];
		uint32_t frontEndFrameCount;
		int totalImageMemory;
		Material* materialHashTable[2048];
		GfxFog fogSettings[5];
		int fogIndex;
		GfxColor color_axis;
		GfxColor color_allies;
		int team;
		trStatistics_t* stats;
		GfxLodParms lodParms;
		GfxLodParms correctedLodParms;
		bool hasAnyImageOverrides;
		bool useSunLightOverride;
		bool useSunDirOverride;
		bool useSunDirLerp;
		float sunLightOverride[3];
		float sunDirOverride[3];
		float sunDirOverrideTarget[3];
		int sunDirLerpBeginTime;
		int sunDirLerpEndTime;
		GfxScaledPlacement identityPlacement;
		GfxViewParms* debugViewParms;
		int endTime;
		bool distortion;
		bool drawSModels;
		bool drawXModels;
		bool drawBModels;
		const char* codeImageNames[27];
		uint32_t viewInfoCount;
		int sunShadowFull;
		float sunShadowmapScale;
		float sunShadowmapScaleNum;
		uint32_t sunShadowSize;
		float sunShadowPartitionRatio;
		int drawSunShadow;
		int skinnedCacheReachedThreshold;
		float waterFloatTime;
	};

	struct CmdArgs
	{
		int nesting;
		int localClientNum[8];
		int controllerIndex[8];
		int argc[8];
		const char** argv[8];
	};

	struct dynBrush_t
	{
		int cmBrushIndex;
		cbrush_t* cmBrush;
		cplane_s* cmPlane[6];
		cLeaf_t* cmLeaf;
	};

	struct dynBrushesArray_t
	{
		dynBrush_t brushes[16];
	};

	struct dynBrushModel_t
	{
		int cmodelIndex;
		cmodel_t* cmodel;
		int entityIndex;
		gentity_s* ent;
		int radBrushIndexForDyn;
		float originalOrigin[3];
	};

	struct dynBrushModelsArray_t
	{
		bool initiated;
		int mapped_bmodels;
		dynBrushModel_t brushes[16];
	};
}

#pragma warning(pop)
