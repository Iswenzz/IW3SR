#include "Engine/Client.hpp"

#include <cmath>

namespace Sim
{
	// com_math.h. cls.frametime is milliseconds, so this is the engine's milliseconds to seconds.
	constexpr float EQUAL_EPSILON = 0.001f;

	// The literal CL_FinishMove multiplies by, which is (float)(65536/360) widened to double.
	// Types.hpp's ANGLE2SHORT does the same scaling in single precision and hands back a signed
	// short, so the engine's constant is kept here rather than reusing that helper.
	constexpr double ANGLE2SHORT_MUL = 182.0444488525391;

	// ClientState carries no m_side / m_forward, so the engine's registered defaults stand in.
	constexpr float m_side = 0.25f;
	constexpr float m_forward = 0.25f;

	// cmd->buttons bits as bg_pmove, bg_jump and bg_weapons actually test them. Types.hpp's
	// UserCmdButtons disagrees on crouch, prone and ads, so the engine's values live here.
	enum CmdButton
	{
		CMD_ATTACK = 0x1,
		CMD_SPRINT = 0x2,
		CMD_PRONE = 0x100,
		CMD_CROUCH = 0x200,
		CMD_JUMP = 0x400,
		CMD_ADS = 0x800,
		CMD_TEMP_STANCE = 0x1000,
	};

	// Client.hpp labels slot 0 KB_RIGHT and slot 1 KB_LEFT, but the game's playersKb is the other
	// way round: +left is slot 0 and +right is slot 1. The engine's meaning is bound here so the
	// yaw and strafe signs come out the way the game computes them.
	constexpr int KB_TURN_LEFT = 0;
	constexpr int KB_TURN_RIGHT = 1;

	static char ClampChar(int i)
	{
		if (i < -128)
			return -128;
		if (i <= 127)
			return static_cast<char>(i);
		return 127;
	}

	// The engine's ftol is an x87 fistp, which rounds to nearest even rather than truncating. Only
	// the m_side path goes through it; every other cast in the input code really does truncate.
	static int SnapFloatToInt(float x)
	{
		return static_cast<int>(std::nearbyint(x));
	}

	// Reads the accumulated mouse delta and hands the other buffer to the next frame's events. The
	// buffer just read is left alone so m_filter can still average it in one frame's time.
	static void CL_GetMouseMovement(ClientState& cl, float* mx, float* my)
	{
		if (cl.m_filter)
		{
			*mx = static_cast<float>((cl.mouseDx[1] + cl.mouseDx[0]) * 0.5f);
			*my = static_cast<float>((cl.mouseDy[1] + cl.mouseDy[0]) * 0.5f);
		}
		else
		{
			*mx = static_cast<float>(cl.mouseDx[cl.mouseIndex]);
			*my = static_cast<float>(cl.mouseDy[cl.mouseIndex]);
		}

		cl.mouseIndex ^= 1;
		cl.mouseDx[cl.mouseIndex] = 0;
		cl.mouseDy[cl.mouseIndex] = 0;
	}

	static void CL_UpdateCmdButton(ClientState& cl, int* cmdButtons, int kbButton, int buttonFlag)
	{
		if (cl.kb[kbButton].active || cl.kb[kbButton].wasPressed)
			*cmdButtons |= buttonFlag;
		cl.kb[kbButton].wasPressed = 0;
	}

	// Fraction of the frame a key was held, and the reason the same held key yields a different
	// forwardmove at a different frame rate. The accumulated msec is drained on every read, so
	// calling this twice on one key in one frame gives the second call nothing.
	float CL_KeyState(ClientState& cl, kbutton_t* key)
	{
		int msec = key->msec;
		key->msec = 0;

		if (key->active)
		{
			// A key with no downtime is treated as held since time zero, which always saturates.
			if (key->downtime)
				msec += cl.com_frameTime - key->downtime;
			else
				msec = cl.com_frameTime;
			key->downtime = cl.com_frameTime;
		}

		if (msec <= 0)
			return 0.0f;

		if (static_cast<unsigned int>(msec) >= cl.frame_msec)
			return 1.0f;

		return static_cast<float>(msec) / static_cast<float>(cl.frame_msec);
	}

	void CL_AdjustAngles(ClientState& cl)
	{
		kbutton_t* kb = cl.kb;

		if ((cl.snapPs.pm_flags & PMF_FROZEN) != 0)
			return;

		double speed;
		if (kb[KB_SPEED].active)
			speed = static_cast<double>(cl.frametime) * EQUAL_EPSILON * cl.cl_anglespeedkey;
		else
			speed = static_cast<double>(cl.frametime) * EQUAL_EPSILON;

		if (!kb[KB_STRAFE].active)
		{
			// cgameMaxYawSpeed caps a turret or vehicle turn. The sim never sets one, so the
			// engine's min() against cl_yawspeed always resolves to cl_yawspeed.
			const float max = cl.cl_yawspeed;
			cl.viewangles[1] =
				static_cast<float>(cl.viewangles[1] - CL_KeyState(cl, &kb[KB_TURN_RIGHT]) * (speed * max));
			cl.viewangles[1] =
				static_cast<float>(CL_KeyState(cl, &kb[KB_TURN_LEFT]) * (speed * max) + cl.viewangles[1]);
		}

		const float maxa = cl.cl_pitchspeed;
		cl.viewangles[0] = static_cast<float>(cl.viewangles[0] - CL_KeyState(cl, &kb[KB_LOOKUP]) * (speed * maxa));
		cl.viewangles[0] = static_cast<float>(CL_KeyState(cl, &kb[KB_LOOKDOWN]) * (speed * maxa) + cl.viewangles[0]);
	}

	void CL_CmdButtons(ClientState& cl, usercmd_s* cmd)
	{
		// The engine walks fourteen kb slots here, of which only these three exist in the sim's
		// kb. CL_KeyMove decides the stance bits again straight after, exactly as it does in game.
		CL_UpdateCmdButton(cl, &cmd->buttons, KB_PRONE, CMD_PRONE);
		CL_UpdateCmdButton(cl, &cmd->buttons, KB_CROUCH, CMD_CROUCH);
		CL_UpdateCmdButton(cl, &cmd->buttons, KB_JUMP, CMD_JUMP);
	}

	void CL_KeyMove(ClientState& cl, usercmd_s* cmd)
	{
		kbutton_t* kb = cl.kb;

		// The engine's temp stance branch, taken while +prone or +movedown is held. Its else side
		// runs the stance state machine, which reduces to a standing player here.
		if (kb[KB_PRONE].active || kb[KB_CROUCH].active || kb[KB_DOWN].active)
		{
			if (kb[KB_PRONE].active)
			{
				cmd->buttons |= CMD_PRONE;
				cmd->buttons &= ~CMD_CROUCH;
			}
			else
			{
				cmd->buttons |= CMD_CROUCH;
				cmd->buttons &= ~CMD_PRONE;
			}
			cmd->buttons |= CMD_TEMP_STANCE;
		}
		else
		{
			cmd->buttons &= ~CMD_PRONE;
			cmd->buttons &= ~CMD_CROUCH;
			cmd->buttons &= ~CMD_TEMP_STANCE;
		}

		// The engine compares +speed against cl->usingAds so the ads toggle inverts the key. With
		// no toggle state here the comparison collapses to the key on its own.
		if (kb[KB_SPEED].active)
			cmd->buttons |= CMD_ADS;
		else
			cmd->buttons &= ~CMD_ADS;

		// A flat 127 either way: there is no walk/run split on the keyboard path.
		int side = static_cast<int>(CL_KeyState(cl, &kb[KB_MOVERIGHT]) * 127.0f);
		side -= static_cast<int>(CL_KeyState(cl, &kb[KB_MOVELEFT]) * 127.0f);
		int forward = static_cast<int>(CL_KeyState(cl, &kb[KB_FORWARD]) * 127.0f);
		forward -= static_cast<int>(CL_KeyState(cl, &kb[KB_BACK]) * 127.0f);

		if (!kb[KB_BACK].active)
		{
			if (kb[KB_SPRINT].active || kb[KB_SPRINT].wasPressed)
			{
				cmd->buttons |= CMD_SPRINT;
				kb[KB_SPRINT].wasPressed = 0;
			}
			else
			{
				cmd->buttons &= ~CMD_SPRINT;
			}
		}

		if (kb[KB_STRAFE].active && (cmd->buttons & CMD_SPRINT) == 0)
		{
			side += static_cast<int>(CL_KeyState(cl, &kb[KB_TURN_RIGHT]) * static_cast<double>(127));
			side -= static_cast<int>(CL_KeyState(cl, &kb[KB_TURN_LEFT]) * static_cast<double>(127));
		}

		cmd->forwardmove = ClampChar(forward);
		cmd->rightmove = ClampChar(side);
	}

	void CL_MouseMove(ClientState& cl, usercmd_s* cmd)
	{
		float mx;
		float my;

		// Runs before every early return: the buffer flip is owed to the next frame even when the
		// frame produces no command at all.
		CL_GetMouseMovement(cl, &mx, &my);

		if (!cl.frame_msec)
			return;

		const float rate =
			static_cast<float>(sqrt(static_cast<double>(my * my + mx * mx)) / static_cast<double>(cl.frame_msec));

		// cgameFOVSensitivityScale is 1 outside a scope, which the sim never enters.
		const float accelSensitivity = rate * cl.cl_mouseAccel + cl.cl_sensitivity;

		if ((cl.snapPs.pm_flags & PMF_FROZEN) != 0)
			return;

		mx = mx * accelSensitivity;
		my = my * accelSensitivity;

		if (mx == 0.0f && my == 0.0f)
			return;

		if (cl.kb[KB_STRAFE].active)
		{
			cmd->rightmove = ClampChar(SnapFloatToInt(mx * m_side) + cmd->rightmove);
		}
		else
		{
			// cgameMaxYawSpeed unset, so the engine's per frame cap on the delta never applies.
			const float delta = cl.m_yaw * mx;
			cl.viewangles[1] = cl.viewangles[1] - delta;
		}

		// The engine also ORs the +mlook key here. That slot is the sim's crouch key, so only
		// cl_freelook decides whether the mouse turns or drives forwardmove.
		if (cl.cl_freelook && !cl.kb[KB_STRAFE].active)
		{
			const float delta = cl.m_pitch * my;
			cl.viewangles[0] = cl.viewangles[0] + delta;
		}
		else
		{
			cmd->forwardmove = ClampChar(cmd->forwardmove - static_cast<int>(my * m_forward));
		}

		// AimAssist_UpdateMouseInput writes both viewangles and the melee charge back over the top
		// of this. There is no aim assist state in ClientState, and it is inert for a mouse.
	}

	void CL_FinishMove(ClientState& cl, usercmd_s* cmd)
	{
		// The engine holds cmd->serverTime to snap.serverTime + 5000 when prediction has run far
		// past the last snapshot. ClientState has no snapshot time, and offline there is nothing
		// for the clamp to bite on, so the predicted time is sent as is.
		cmd->serverTime = cl.serverTime;

		// No delta_angles here on purpose: cl.viewangles is already kept in command space, and
		// PM_UpdateViewAngles_Clamp is what adds ps->delta_angles back. Folding it in at pack time
		// would count it twice. cgameKickAngles is zero without a weapon firing.
		for (int i = 0; i < 3; ++i)
			cmd->angles[i] = static_cast<uint16_t>(static_cast<int>(cl.viewangles[i] * ANGLE2SHORT_MUL));
	}

	usercmd_s CL_CreateCmd(ClientState& cl)
	{
		const float oldAngles = cl.viewangles[0];
		CL_AdjustAngles(cl);

		usercmd_s cmd = {};
		CL_CmdButtons(cl, &cmd);
		CL_KeyMove(cl, &cmd);
		CL_MouseMove(cl, &cmd);

		// One command may not swing the pitch more than 90 degrees, whatever the mouse delivered.
		if (cl.viewangles[0] - oldAngles <= 90.0)
		{
			if (oldAngles - cl.viewangles[0] > 90.0)
				cl.viewangles[0] = oldAngles - 90.0f;
		}
		else
		{
			cl.viewangles[0] = oldAngles + 90.0f;
		}

		CL_FinishMove(cl, &cmd);
		return cmd;
	}

	void CL_CreateNewCommands(ClientState& cl)
	{
		const int cmdNum = ++cl.cmdNumber & 0x7F;
		cl.cmds[cmdNum] = CL_CreateCmd(cl);
	}

	void CL_RunOncePerClientFrame(ClientState& cl, int msec)
	{
		cl.realFrametime = msec;
		cl.frametime = msec;
		cl.realtime += msec;

		// frame_msec is the width CL_KeyState divides by, and it comes off com_frameTime rather
		// than off msec: it is the wall clock gap between limiter exits, not the credited frame.
		cl.frame_msec = static_cast<unsigned int>(cl.com_frameTime - cl.old_com_frameTime);
		if (cl.com_frameTime == cl.old_com_frameTime)
			cl.frame_msec = 1;
		if (cl.frame_msec > 200)
			cl.frame_msec = 200;
		cl.old_com_frameTime = cl.com_frameTime;
	}

	void KeyDown(ClientState&, kbutton_t* key, int keyCode, int downTime)
	{
		if (keyCode == key->down[0] || keyCode == key->down[1])
			return;

		if (key->down[0])
		{
			// Three keys on one button is the engine's give up case: it drops the press whole.
			if (key->down[1])
				return;
			key->down[1] = keyCode;
		}
		else
		{
			key->down[0] = keyCode;
		}

		// A second key on an already held button does not restart downtime.
		if (!key->active)
		{
			key->downtime = downTime;
			key->active = 1;
			key->wasPressed = 1;
		}
	}

	void KeyUp(ClientState& cl, kbutton_t* key, int keyCode, int upTime)
	{
		// The engine's empty key code path, which clears the button outright and credits no time.
		if (keyCode < 0)
		{
			key->down[1] = 0;
			key->down[0] = 0;
			key->active = 0;
			return;
		}

		if (key->down[0] == keyCode)
		{
			key->down[0] = 0;
		}
		else
		{
			if (key->down[1] != keyCode)
				return;
			key->down[1] = 0;
		}

		if (key->down[0] || key->down[1])
			return;

		key->active = 0;

		// A release with no timestamp is credited half a frame. That guess is what lets a key held
		// for less than one frame still move the player.
		if (upTime)
			key->msec = key->msec + upTime - key->downtime;
		else
			key->msec = key->msec + static_cast<int>(cl.frame_msec >> 1);
	}
}
