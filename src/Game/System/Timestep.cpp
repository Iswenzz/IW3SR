#include "Timestep.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Schedule.hpp"
#include "Game/System/System.hpp"

namespace IW3SR
{
	// The frame limiter reads com_maxfps through this pointer instead of the dvar table, so
	// pointing it at a copy of the dvar caps frames without touching what com_maxfps reads as.
	constexpr uintptr_t ComMaxFpsRef = 0x1476EF8;

	// A packet carries at most 32 commands and one goes out per rendered frame, so this is a hard
	// ceiling rather than a tuning choice: past it the packet writer would drop commands the server
	// never sees, which desyncs it from us. Dropping movement time instead at least keeps us agreed.
	constexpr int MaxSteps = 32;

	// One clamped frame is a hitch. This many in a row is a configuration that cannot carry the rate.
	constexpr int StarvedLimit = 30;

	// Length of the self test.
	constexpr int TestLength = 3000;
	// Gap between the three keys going down, so the hop starts the way a player starts one.
	constexpr int TestStagger = 200;

	// What a frame at com_maxfps would spend on its own work, before the limiter is consulted.
	// Not measurable from a client rendering at sr_maxfps, so it is a constant rather than a knob:
	// a knob here would let someone dial the movement rate back up to one no vanilla client reaches.
	constexpr int FrameWork = 500;

	// How many times a one millisecond sleep is timed at startup to find what it really costs.
	constexpr int SleepSamples = 33;

	// Past this the movement clock is stale, from a map change or a server time correction.
	constexpr int MaxDrift = 500;

	void Timestep::Initialize()
	{
		ComMaxFps = Dvar::Find("com_maxfps");

		// Movement here is not proven identical to a client actually running at com_maxfps, so it stays
		// out of a release build entirely rather than sitting behind a dvar someone could turn on.
		if (!System::IsDebug())
			return;

		Enabled = Dvar::RegisterBool("sr_timestep", DVAR_SAVED,
			"Run movement on a fixed timestep taken from com_maxfps, whatever the frame rate is", true);
		MaxFps =
			Dvar::RegisterInt("sr_maxfps", DVAR_SAVED, "Frame rate cap, 0 to follow com_maxfps", DisplayFps(), 0, 1000);
		Smooth = Dvar::RegisterBool("sr_timestep_smooth", DVAR_SAVED,
			"Draw the view at the frame time instead of at the last movement step", true);
		Log = Dvar::RegisterBool("sr_timestep_log", DVAR_TEMP,
			"Write every movement command to iw3sr/Logs/timestep.csv", false);

		Pace.Sleep = MeasureSleep();
		Pace.Work = FrameWork;

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
		Vanilla = {};
	}

	// What the audit has seen so far, so the absence of a warning can be read as evidence rather
	// than taken on trust.
	void Timestep::Status()
	{
		if (!Active())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Timestep is off, movement follows the frame rate.\n", 0);
			return;
		}
		const int movement = MovementFps();
		const int step = 1000 / movement;

		// The achieved rate, not com_maxfps. A vanilla client's limiter overshoots its target by
		// whatever a sleep costs, and the steps here carry the same overshoot, so the two differ.
		const int emitted = Steady + Wobble;
		const int reached = emitted ? 1000 * emitted / std::max(1, Emitted - First) : 0;

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Timestep: com_maxfps {} is a {} ms step, drawing at {} fps.\n"
						"Reaching {} steps a second, which is what a vanilla client here would.\n"
						"A one millisecond sleep costs {} us. Steps on cadence: {}, off: {}.\n",
				movement, step, RenderFps(), reached, Pace.Sleep, Steady, Wobble)
				.c_str(),
			0);
	}

	// Runs while you play, so a break in either property shows up on its own rather than waiting for
	// someone to go looking for it.
	void Timestep::Audit()
	{
		if (Warned || Steady + Wobble < 600 || Wobble * 8 < Steady)
			return;

		Warned = true;
		Com_PrintMessage(CON_CHANNEL_ERROR,
			std::format("^1Timestep: {} of {} steps are wider than com_maxfps ever sends, movement will "
						"not match it. Please report this.\n",
				Wobble, Steady + Wobble)
				.c_str(),
			0);
	}

	// Writes the commands as they leave for the server, so a run with the timestep on and one with it
	// off can be compared on what pmove was actually handed rather than on how the two felt.
	void Timestep::Record(const usercmd_s& cmd)
	{
		if (!Log)
			return;

		if (!Log->current.enabled)
		{
			if (!Journal.is_open())
				return;

			Journal.close();
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Wrote {} movement commands to iw3sr/Logs\n", Logged).c_str(), 0);
			return;
		}

		if (!Journal.is_open())
		{
			const char* name = Enabled && Enabled->current.enabled ? "timestep_on.csv" : "timestep_off.csv";
			const std::filesystem::path path = Environment::Path(Directory::App) / "Logs";

			std::error_code ec;
			std::filesystem::create_directories(path, ec);

			Journal.open(path / name, std::ios::trunc);
			if (!Journal.is_open())
				return;

			Journal << "serverTime,msec,forwardmove,rightmove,buttons,yaw,frameTime,sysMsgTime,speed,ground\n";
			Logged = 0;
			Previous = cmd.serverTime;
		}
		Journal << cmd.serverTime << ',' << cmd.serverTime - Previous << ',' << int(cmd.forwardmove) << ','
				<< int(cmd.rightmove) << ',' << cmd.buttons << ',' << cmd.angles[1] << ',' << com_frameTime << ','
				<< (g_wv ? g_wv->sysMsgTime : 0) << ','
				<< static_cast<int>(glm::length(vec2(cgs->predictedPlayerState.velocity))) << ','
				<< (cgs->predictedPlayerState.groundEntityNum != ENTITYNUM_NONE ? 1 : 0) << '\n';

		Previous = cmd.serverTime;
		Logged++;
	}

	// Plays a fixed key pattern to the client so a run with the timestep on and one with it off see
	// the exact same input. Hands cannot repeat an A/D spam closely enough to compare the two.
	void Timestep::StartTest()
	{
		if (!Log || Test)
			return;

		// Scheduled on the clock the steps are placed on. com_frameTime drifts from it, because
		// realtime gains the padding the limiter adds to a frame that finished early.
		Test = cls->realtime;
		Beat = 0;
		Peak = 0;
		Log->current.enabled = true;

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Timestep test running for 3 seconds, keep off the keyboard.\n", 0);
	}

	void Timestep::TestFrame()
	{
		if (!Test)
			return;

		const int now = cls->realtime - Test;

		// Speed is what a difference in stepping shows up as, so the test reports it rather than
		// leaving it to how the landing felt.
		const int speed = static_cast<int>(glm::length(vec2(cgs->predictedPlayerState.velocity)));
		Peak = speed > Peak ? speed : Peak;

		if (now > TestLength)
		{
			Send("-forward", 200, Test + TestLength);
			Send("-gostand", 203, Test + TestLength);
			Send("-moveright", 202, Test + TestLength);

			Test = 0;
			Log->current.enabled = false;

			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, std::format("Timestep test done, peak speed {}.\n", Peak).c_str(),
				0);
			return;
		}
		// Forward, then jump, then strafe, each held for the rest of the run and staggered the way a
		// player starts a hop. Nothing is released, so nothing here depends on a key reversal.
		if (!Beat)
		{
			Send("+forward", 200, Test);
			Beat = 1;
		}
		while (Beat < 3 && Beat * TestStagger <= now)
		{
			const int time = Test + Beat * TestStagger;

			Beat == 1 ? Send("+gostand", 203, time) : Send("+moveright", 202, time);
			Beat++;
		}
	}

	void Timestep::Send(const char* command, int key, int time)
	{
		// Stamped on the window message clock the way the engine stamps a real key event, so the
		// test goes down the same path with the same degenerate key timing as a real press.
		const int offset = static_cast<int>(GetTickCount()) - cls->realtime;
		std::string text = std::format("{} {} {}", command, key, time + offset);
		GSystem::ExecuteSingleCommand(0, 0, text.data());
	}

	// Key binds have already run by the time this is called, in the event pump at the top of the
	// frame, so a key latches at the start of the frame it arrived in, which is where vanilla
	// latches it. Placing events inside the frame by their timestamps was tried and reverted: the
	// window message clock they carry ticks at 15.6ms on modern Windows, wider than a whole frame,
	// and the placement jitter it caused was worse than latching up to a frame early.
	void Timestep::CreateNewCommands(int localClientNum)
	{
		TestFrame();

		if (!Active())
		{
			Time = 0;

			const int slot = clients->cmdNumber;
			CL_CreateNewCommands_h(localClientNum);

			if (clients->cmdNumber != slot)
				Record(clients->cmds[clients->cmdNumber & 0x7F]);
			return;
		}
		Split(localClientNum);
	}

	// The last movement step sits up to a step short of the frame being drawn, by a gap that
	// changes frame to frame, which reads as judder no client stepping at its frame rate shows.
	// Carrying the view along the velocity for the remainder draws it at the frame time. The gun
	// was anchored to the view before the carry, so it moves by the same amount, or it would
	// judder against the camera instead of against the world. The refdef is rebuilt every frame
	// and movement never reads it; of the engine, only the sound listener and the melee assist
	// see the carried eye, each by under a step of travel.
	void Timestep::CalcViewValues(int localClientNum)
	{
		CG_CalcViewValues_h(localClientNum);

		if (!Active() || !Smooth || !Smooth->current.enabled)
			return;

		const playerState_s& ps = cgs->predictedPlayerState;

		// Only a state predicted up to the newest command may be carried further. One that was
		// interpolated instead, following another player or after death, sits a snapshot behind,
		// and carrying it would throw the view around rather than settle it.
		if (ps.commandTime != clients->cmds[clients->cmdNumber & 0x7F].serverTime)
			return;
		if (ps.pm_type != PM_NORMAL && ps.pm_type != PM_NOCLIP && ps.pm_type != PM_UFO && ps.pm_type != PM_SPECTATOR)
			return;

		const int lag = clients->serverTime - ps.commandTime;
		if (lag <= 0 || lag > 1000 / MovementFps() + 1)
			return;

		const vec3 carry = ps.velocity * (static_cast<float>(lag) * 0.001f);

		cgs->refdef.vieworg += carry;
		for (int i = 0; i < 3; i++)
			cgs->viewModelAxis[3][i] += carry[i];
	}

	// Builds one command per movement step by running the engine's own builder that many times, each
	// with the clocks it reads pointed at that step. Sampling input per step rather than per frame is
	// what a client really running at com_maxfps does, and it has to be done this way round rather
	// than by copying one command: CL_KeyState divides a key's held time by frame_msec, so a long
	// frame yields a weaker forwardmove and rightmove than the same keys held at com_maxfps would.
	// What a one millisecond sleep really costs here. Vanilla's limiter overshoot is entirely this
	// number, so it is measured once rather than assumed. Sleep stands in for the engine's
	// NET_Sleep(1): both are bounded by the same timer granularity.
	int Timestep::MeasureSleep()
	{
		LARGE_INTEGER frequency = {};
		if (!QueryPerformanceFrequency(&frequency) || !frequency.QuadPart)
			return 1000;

		int samples[SleepSamples] = {};
		for (int i = 0; i < SleepSamples; i++)
		{
			LARGE_INTEGER before = {};
			LARGE_INTEGER after = {};

			QueryPerformanceCounter(&before);
			::Sleep(1);
			QueryPerformanceCounter(&after);

			samples[i] = static_cast<int>((after.QuadPart - before.QuadPart) * 1000000 / frequency.QuadPart);
		}
		std::sort(std::begin(samples), std::end(samples));

		// The median, so one descheduled sample cannot decide the movement rate.
		return std::clamp(samples[SleepSamples / 2], 1000, 8000);
	}

	void Timestep::Split(int localClientNum)
	{
		const int step = 1000 / MovementFps();

		// serverTime is built as realtime plus a delta the engine walks toward the server a
		// millisecond at a time, so stepping realtime and adding that delta rebuilds it the way
		// vanilla does, walk included. Stepping com_frameTime instead would add a millisecond of our
		// own whenever 1000/fps is not whole, and hand pmove a step vanilla never sends.
		const int target = cls->realtime;
		const int delta = clients->serverTime - target;

		int widths[MaxSteps] = {};
		const Steps plan = PlanSteps(Vanilla, widths, MaxSteps, step, target, cls->frametime, MaxDrift, Pace);
		const int count = plan.Count;

		Time = plan.Clock;

		if (plan.Starved)
		{
			// Silently running slow is the one failure that would follow a run onto the server.
			if (++Starved == StarvedLimit)
			{
				Com_PrintMessage(CON_CHANNEL_ERROR,
					std::format("^1com_maxfps {} needs more than 32 commands per frame at {} fps and cannot "
								"be delivered. Raise sr_maxfps above {} or lower com_maxfps.\n",
						MovementFps(), RenderFps(), (MovementFps() + 31) / 32)
						.c_str(),
					0);
			}
		}
		else
		{
			Starved = 0;
		}
		// Frames are outrunning the movement rate, so this one owes no command at all.
		if (!count)
			return;

		const int serverTime = clients->serverTime;
		const int frameTime = com_frameTime;
		const int frametime = cls->frametime;
		const int msec = frame_msec;

		const int dx = clients->mouseDx[clients->mouseIndex];
		const int dy = clients->mouseDy[clients->mouseIndex];

		int time = Time;
		for (int i = 1; i <= count; i++)
		{
			const int width = widths[i - 1];
			time += width;

			// Key timing lives on this clock, so a step placed on it measures a held key over the
			// step rather than over the whole frame.
			com_frameTime = time;
			clients->serverTime = time + delta;
			cls->frametime = width;
			frame_msec = width;

			// The frame's mouse travel belongs to the frame, so hand each step its own share of it.
			clients->mouseDx[clients->mouseIndex] = dx * i / count - dx * (i - 1) / count;
			clients->mouseDy[clients->mouseIndex] = dy * i / count - dy * (i - 1) / count;

			CL_CreateNewCommands_h(localClientNum);
			Record(clients->cmds[clients->cmdNumber & 0x7F]);

			// Vanilla steps wander by the millisecond the engine walks the clock. Wider than that is
			// jitter of our own making, and a step com_maxfps never produces.
			const int stamp = time + delta;
			if (!First)
				First = stamp;
			if (Emitted)
				std::abs(stamp - Emitted - width) > 1 ? Wobble++ : Steady++;
			Emitted = stamp;
		}

		Time = time;
		Audit();

		clients->serverTime = serverTime;
		com_frameTime = frameTime;
		cls->frametime = frametime;
		frame_msec = msec;
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
