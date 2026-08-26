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

	// Length of the self test, and the beat it swaps strafe keys on. 37ms is deliberately not a
	// multiple of any step size, so every swap lands part way through a movement step.
	constexpr int TestLength = 3000;
	// Gap between the three keys going down, so the hop starts the way a player starts one.
	constexpr int TestStagger = 200;

	// Past this the movement clock is stale, from a map change or a server time correction.
	constexpr int MaxDrift = 500;

	// Enough for any human hand; a queue growing past it means the steps stopped draining it.
	constexpr size_t MaxEvents = 64;

	void Timestep::Initialize()
	{
		// Movement here is not proven identical to a client actually running at com_maxfps, so it stays
		// out of a release build entirely rather than sitting behind a dvar someone could turn on.
		if (!System::IsDebug())
			return;

		Enabled = Dvar::RegisterBool("sr_timestep", DVAR_SAVED,
			"Run movement on a fixed timestep taken from com_maxfps, whatever the frame rate is", true);
		MaxFps = Dvar::RegisterInt("sr_maxfps", DVAR_SAVED, "Frame rate cap, 0 to follow com_maxfps",
			DisplayFps(), 0, 1000);
		Log = Dvar::RegisterBool("sr_timestep_log", DVAR_TEMP,
			"Write every movement command to iw3sr/Logs/timestep.csv", false);

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
		Flush();
	}

	// Holds back the key events the engine would otherwise apply to the whole frame at once. Their
	// timestamps are what a client running at com_maxfps sorts them into commands by, so replaying
	// them per step is what keeps forwardmove and rightmove identical to that client.
	bool Timestep::Defer(int localClientNum, int controllerIndex, const std::string& command)
	{
		if (Replaying || Events.size() >= MaxEvents || !Active())
			return false;
		if (command.empty() || (command.front() != '+' && command.front() != '-'))
			return false;

		// Only a bind carries the key and the millisecond it happened. Anything else is a console
		// command with no place on the movement clock.
		const size_t key = command.find(' ');
		if (key == std::string::npos)
			return false;

		const size_t stamp = command.find(' ', key + 1);
		if (stamp == std::string::npos)
			return false;

		int time = 0;
		const char* first = command.data() + stamp + 1;
		const auto [ptr, ec] = std::from_chars(first, command.data() + command.size(), time);

		if (ec != std::errc{} || time <= 0)
			return false;

		// The engine stamps key events off the window message clock, which counts from when Windows
		// booted, while com_frameTime counts from when the game started. Left alone the two never
		// compare, and an event days ahead of every step parks itself at the head of the queue.
		Events.push_back({ command, localClientNum, controllerIndex, time - Offset });
		return true;
	}

	// A key event belongs to the step its timestamp falls in. One that cannot be placed there lands
	// on the wrong side of a movement step, which is exactly the difference between matching vanilla
	// and not, so the two cases are counted rather than both quietly working.
	void Timestep::Replay(int time, bool placed)
	{
		while (!Events.empty() && Events.front().Time <= time)
		{
			KeyEvent event = std::move(Events.front());
			Events.pop_front();

			placed ? Placed++ : Late++;

			Replaying = true;
			GSystem::ExecuteSingleCommand(event.LocalClientNum, event.ControllerIndex, event.Command.data());
			Replaying = false;
		}
	}

	void Timestep::Flush()
	{
		Replay(std::numeric_limits<int>::max(), false);

		// The odd one at a frame edge is normal. Mostly late means the timestamps no longer compare
		// with the step clock, and movement has quietly stopped matching a client running at com_maxfps.
		if (Warned || Late < 16 || Late <= Placed)
			return;

		Warned = true;
		Com_PrintMessage(CON_CHANNEL_ERROR,
			"^1Timestep: key input is not landing on its movement step, movement will not match "
			"com_maxfps. Please report this.\n", 0);
	}

	// What the two checks have seen so far, so the absence of a warning can be read as evidence
	// rather than taken on trust.
	void Timestep::Status()
	{
		if (!Active())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Timestep is off, movement follows the frame rate.\n", 0);
			return;
		}
		const int movement = MovementFps();
		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Timestep: {} steps a second of {} ms, drawing at {} fps.\n"
						"Key input on its own step: {}, late: {}.\n"
						"Steps com_maxfps could produce: {}, wider: {}.\n",
				movement, 1000 / movement, RenderFps(), Placed, Late, Steady, Wobble)
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
						"not match it. Please report this.\n", Wobble, Steady + Wobble)
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

			Journal << "serverTime,msec,forwardmove,rightmove,buttons,yaw,frameTime,queued,head,sysMsgTime,speed,ground\n";
			Logged = 0;
			Previous = cmd.serverTime;
		}
		Journal << cmd.serverTime << ',' << cmd.serverTime - Previous << ',' << int(cmd.forwardmove) << ','
				<< int(cmd.rightmove) << ',' << cmd.buttons << ',' << cmd.angles[1] << ',' << com_frameTime << ','
				<< Events.size() << ',' << (Events.empty() ? 0 : Events.front().Time) << ','
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

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			"Timestep test running for 3 seconds, keep off the keyboard.\n", 0);
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

			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Timestep test done, peak speed {}.\n", Peak).c_str(), 0);
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
		// Stamped the way the engine stamps a real key event, so the test goes down the same path.
		std::string text = std::format("{} {} {}", command, key, time + Offset);
		GSystem::ExecuteSingleCommand(0, 0, text.data());
	}

	void Timestep::CreateNewCommands(int localClientNum)
	{
		Offset = static_cast<int>(GetTickCount()) - cls->realtime;

		TestFrame();

		if (!Active())
		{
			Time = 0;
			Flush();

			const int slot = clients->cmdNumber;
			CL_CreateNewCommands_h(localClientNum);

			if (clients->cmdNumber != slot)
				Record(clients->cmds[clients->cmdNumber & 0x7F]);
			return;
		}
		Split(localClientNum);
	}

	// Builds one command per movement step by running the engine's own builder that many times, each
	// with the clocks it reads pointed at that step. Sampling input per step rather than per frame is
	// what a client really running at com_maxfps does, and it has to be done this way round rather
	// than by copying one command: CL_KeyState divides a key's held time by frame_msec, so a long
	// frame yields a weaker forwardmove and rightmove than the same keys held at com_maxfps would.
	void Timestep::Split(int localClientNum)
	{
		const int step = 1000 / MovementFps();

		// serverTime is built as realtime plus a delta the engine walks toward the server a
		// millisecond at a time, so stepping realtime and adding that delta rebuilds it the way
		// vanilla does, walk included. Stepping com_frameTime instead would add a millisecond of our
		// own whenever 1000/fps is not whole, and hand pmove a step vanilla never sends.
		const int target = cls->realtime;
		const int delta = clients->serverTime - target;
		const Steps plan = PlanSteps(Time, target, step, MaxSteps, MaxDrift);

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
		if (!plan.Count)
			return;

		const int serverTime = clients->serverTime;
		const int frameTime = com_frameTime;
		const int frametime = cls->frametime;
		const int msec = frame_msec;

		const int dx = clients->mouseDx[clients->mouseIndex];
		const int dy = clients->mouseDy[clients->mouseIndex];

		for (int i = 1; i <= plan.Count; i++)
		{
			const int time = Time + i * step;

			// Key timing lives on this clock, so a step placed on it measures a held key over the
			// step rather than over the whole frame.
			com_frameTime = time;
			clients->serverTime = time + delta;
			cls->frametime = step;
			frame_msec = step;

			Replay(com_frameTime, true);

			// The frame's mouse travel belongs to the frame, so hand each step its own share of it.
			clients->mouseDx[clients->mouseIndex] = dx * i / plan.Count - dx * (i - 1) / plan.Count;
			clients->mouseDy[clients->mouseIndex] = dy * i / plan.Count - dy * (i - 1) / plan.Count;

			CL_CreateNewCommands_h(localClientNum);
			Record(clients->cmds[clients->cmdNumber & 0x7F]);

			// Vanilla steps wander by the millisecond the engine walks the clock. Wider than that is
			// jitter of our own making, and a step com_maxfps never produces.
			const int stamp = time + delta;
			if (Emitted)
				std::abs(stamp - Emitted - step) > 1 ? Wobble++ : Steady++;
			Emitted = stamp;
		}

		// Whatever is left belongs to this frame and the engine would have applied all of it at the
		// start of it, so none may survive. Draining by timestamp alone is not enough: an event
		// stamped from another time base is never late enough to release, and parks itself at the
		// head where it holds back everything queued behind it.
		Flush();

		Time += plan.Count * step;
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
