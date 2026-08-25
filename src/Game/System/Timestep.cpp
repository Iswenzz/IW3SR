#include "Timestep.hpp"

#include "Game/Player/PMove.hpp"
#include "Game/System/Dvar.hpp"

namespace IW3SR
{
	// The frame limiter reads com_maxfps through this pointer instead of the dvar table, so
	// pointing it at a copy of the dvar caps frames without touching what com_maxfps reads as.
	constexpr uintptr_t ComMaxFpsRef = 0x1476EF8;

	// A packet carries at most 32 commands, and dropping movement time keeps the server in step
	// with us where dropping commands would not, so a hitch is spent rather than queued past that.
	constexpr int MaxSteps = 32;

	// Past this the movement clock is stale, from a map change or a server time correction.
	constexpr int MaxDrift = 500;

	void Timestep::Initialize()
	{
		Enabled = Dvar::RegisterBool("sr_timestep", DVAR_SAVED,
			"Run movement on a fixed timestep taken from com_maxfps, whatever the frame rate is", true);
		MaxFps = Dvar::RegisterInt("sr_maxfps", DVAR_SAVED, "Frame rate cap, 0 to follow com_maxfps",
			DisplayFps(), 0, 1000);

		ComMaxFps = Dvar::Find("com_maxfps");
		const auto limiter = reinterpret_cast<dvar_s**>(ComMaxFpsRef);

		// Leave the limiter alone rather than guess if the engine no longer keeps it here.
		if (!ComMaxFps || *limiter != ComMaxFps)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1Frame limiter not found, sr_maxfps will do nothing.\n", 0);
			return;
		}

		Limit = *ComMaxFps;
		Limiter = limiter;
		*Limiter = &Limit;
	}

	void Timestep::Frame()
	{
		if (!Limiter)
			return;
		Limit.current.integer = RenderFps();
	}

	void Timestep::Reset()
	{
		Time = 0;
	}

	void Timestep::CreateNewCommands(int localClientNum)
	{
		const int slot = clients->cmdNumber + 1;

		// A frame that owes no command has to leave this slot holding what it held before. The
		// engine reads the same slot back as the oldest command to decide the connection is dead.
		const usercmd_s previous = *PMove::GetUserCommand(slot);
		CL_CreateNewCommands_h(localClientNum);

		// The engine queues nothing until the first snapshots are in.
		if (clients->cmdNumber != slot)
			return;

		if (!Active())
		{
			Time = 0;
			return;
		}
		Split(slot, previous);
	}

	// Rewrites the command the engine just queued into as many fixed size steps as have come due,
	// spreading the frame's view rotation over them so a slow frame still turns at its own pace.
	void Timestep::Split(int slot, const usercmd_s& previous)
	{
		const int step = 1000 / MovementFps();
		const usercmd_s cmd = *PMove::GetUserCommand(slot);
		const int elapsed = cmd.serverTime - Time;

		if (!Time || elapsed < 0 || elapsed > MaxDrift)
		{
			Time = cmd.serverTime - step;
			std::copy_n(cmd.angles, 3, Angles);
		}
		int steps = (cmd.serverTime - Time) / step;

		if (steps > MaxSteps)
		{
			steps = MaxSteps;
			Time = cmd.serverTime - steps * step;
		}
		if (steps <= 0)
		{
			// Frames are outrunning the movement rate, so this one owes no command at all.
			*PMove::GetUserCommand(slot) = previous;
			clients->cmdNumber = slot - 1;
			return;
		}
		for (int i = 1; i <= steps; i++)
		{
			usercmd_s& out = *PMove::GetUserCommand(slot + i - 1);
			out = cmd;
			out.serverTime = Time + i * step;

			for (int axis = 0; axis < 3; axis++)
			{
				const auto from = static_cast<int16_t>(Angles[axis]);
				const auto delta = static_cast<int16_t>(cmd.angles[axis] - Angles[axis]);
				out.angles[axis] = static_cast<uint16_t>(from + delta * i / steps);
			}
		}
		Time += steps * step;
		std::copy_n(cmd.angles, 3, Angles);
		clients->cmdNumber = slot + steps - 1;
	}

	bool Timestep::Active()
	{
		return Enabled && Enabled->current.enabled && MovementFps() > 0 && !clc.demoplaying
			&& client_ui->connectionState == CA_ACTIVE;
	}

	int Timestep::MovementFps()
	{
		return ComMaxFps ? ComMaxFps->current.integer : 0;
	}

	// With the timestep off com_maxfps has to cap frames again, otherwise turning it off leaves the
	// game running at sr_maxfps with movement following it, which is no different from leaving it on.
	int Timestep::RenderFps()
	{
		const int cap = MaxFps ? MaxFps->current.integer : 0;
		if (cap <= 0 || !Enabled || !Enabled->current.enabled)
			return MovementFps();
		return cap;
	}

	// Refresh rate of the monitor the game sits on, so a slow second display cannot decide the cap.
	int Timestep::DisplayFps()
	{
		MONITORINFOEX monitor = {};
		monitor.cbSize = sizeof(monitor);

		DEVMODE mode = {};
		mode.dmSize = sizeof(mode);

		const HMONITOR handle = MonitorFromWindow(g_wv ? g_wv->hWnd : nullptr, MONITOR_DEFAULTTOPRIMARY);
		const char* device = GetMonitorInfo(handle, &monitor) ? monitor.szDevice : nullptr;

		// A frequency of 0 or 1 means the default rate of the hardware, which is anyones guess.
		if (!EnumDisplaySettings(device, ENUM_CURRENT_SETTINGS, &mode) || mode.dmDisplayFrequency < 2)
			return 60;
		return static_cast<int>(mode.dmDisplayFrequency);
	}
}
