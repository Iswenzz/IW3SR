#pragma once
// The client state the command builder reads and writes, gathered into one object so a driver can
// own a whole client and poke its clocks the way Timestep::Split pokes the game's globals.
#include "Engine/Types.hpp"

namespace Sim
{
	using namespace IW3SR;

	// Slots in ClientState::kb. These are the simulator's own, not the game's playersKb indices:
	// nothing is addressed by raw number, so a slot only has to mean the same thing to the hand
	// pressing it and to the command builder reading it.
	enum KeyButton
	{
		KB_LEFT = 0,
		KB_RIGHT = 1,
		KB_FORWARD = 2,
		KB_BACK = 3,
		KB_LOOKUP = 4,
		KB_LOOKDOWN = 5,
		KB_MOVELEFT = 6,
		KB_MOVERIGHT = 7,
		KB_STRAFE = 8,
		KB_SPEED = 9,
		KB_UP = 10,
		KB_DOWN = 11,
		KB_JUMP = 12,
		KB_CROUCH = 13,
		KB_PRONE = 14,
		KB_SPRINT = 15,
		KB_COUNT = 16,
	};

	// The engine's kbutton_t. down[] holds the two key codes currently holding the button.
	struct kbutton_t
	{
		int down[2];
		int downtime;
		int msec;
		int active;
		int wasPressed;
	};

	// Everything CL_CreateCmd touches, plus the frame clocks it reads. Field names follow the
	// engine globals they stand for so the port reads against KisakCOD line by line.
	struct ClientState
	{
		// clientActive_t
		kbutton_t kb[KB_COUNT] = {};
		vec3 viewangles = {};
		int mouseDx[2] = {};
		int mouseDy[2] = {};
		int mouseIndex = 0;
		usercmd_s cmds[128] = {};
		int cmdNumber = 0;
		int serverTime = 0;
		int serverTimeDelta = 0;
		playerState_s snapPs = {};

		// clientStatic_t
		int realtime = 0;
		int frametime = 0;
		int realFrametime = 0;

		// qcommon
		int com_frameTime = 0;
		int old_com_frameTime = 0;
		unsigned int frame_msec = 1;

		// dvars the input path reads
		float cl_sensitivity = 5.0f;
		float cl_mouseAccel = 0.0f;
		float cl_yawspeed = 140.0f;
		float cl_pitchspeed = 140.0f;
		float cl_anglespeedkey = 1.5f;
		int m_filter = 0;
		float m_yaw = 0.022f;
		float m_pitch = 0.022f;
		int cl_freelook = 1;
	};

	// Ports of the engine's command builder. Signatures mirror the originals with the globals
	// threaded through ClientState instead.
	float CL_KeyState(ClientState& cl, kbutton_t* key);
	void CL_AdjustAngles(ClientState& cl);
	void CL_CmdButtons(ClientState& cl, usercmd_s* cmd);
	void CL_KeyMove(ClientState& cl, usercmd_s* cmd);
	void CL_MouseMove(ClientState& cl, usercmd_s* cmd);
	void CL_FinishMove(ClientState& cl, usercmd_s* cmd);
	usercmd_s CL_CreateCmd(ClientState& cl);
	void CL_CreateNewCommands(ClientState& cl);

	// CL_RunOncePerClientFrame's clock bookkeeping: cls.frametime, cls.realtime and frame_msec.
	void CL_RunOncePerClientFrame(ClientState& cl, int msec);

	// Key_Event's button bookkeeping, so a scripted press goes down the same path a real one does.
	void KeyDown(ClientState& cl, kbutton_t* key, int keyCode, int downTime);
	void KeyUp(ClientState& cl, kbutton_t* key, int keyCode, int upTime);
}
