#include "Timestep.hpp"

#include "Engine/Core/Utils/StringUtils.hpp"

#include "Game/System/Client.hpp"
#include "Game/System/Dvar.hpp"
#include "Game/System/System.hpp"

namespace IW3SR
{
	// The frame limiter reads com_maxfps through this pointer instead of the dvar table, so
	// pointing it at a copy of the dvar caps frames without touching what com_maxfps reads as.
	constexpr uintptr_t ComMaxFpsRef = 0x1476EF8;

	// A packet carries at most 32 commands and one goes out per rendered frame, so this is a hard
	// ceiling rather than a tuning choice: past it the packet writer would drop commands the server
	// never sees, which desyncs it from us.
	constexpr int MaxSteps = 32;

	// One clamped frame is a hitch. This many in a row is a configuration that cannot carry the rate.
	constexpr int StarvedLimit = 30;

	// Length of the self test.
	constexpr int TestLength = 3000;
	// Gap between the three keys going down, so the hop starts the way a player starts one.
	constexpr int TestStagger = 200;

	// Past this the movement clock is stale, from a map change or a server time correction.
	constexpr int MaxDrift = 500;

	void Timestep::Initialize()
	{
		ComMaxFps = Dvar::Find("com_maxfps");

		// Stays out of a release build until it is confirmed in play, rather than sitting behind a dvar
		// someone could turn on.
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
		Apex = Dvar::RegisterBool("sr_timestep_apex", DVAR_SAVED,
			"Print how high every jump peaks next to what a standing jump at com_maxfps reaches", false);

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
		Vanilla = {};
		Stepped = 0;
		Emitted = 0;
		First = 0;
		Sent = 0;
		Dropping = false;
		Grounded = true;
		Jumping = false;
	}

	bool Timestep::Command(const std::string& command)
	{
		if (!Enabled)
			return false;

		std::istringstream stream(command);
		std::string verb;
		stream >> verb;
		verb = StringUtils::ToLower(verb);

		if (verb == "sr_timestep_test")
		{
			StartTest();
			return true;
		}
		if (verb == "sr_timestep_status")
		{
			Status();
			return true;
		}
		return false;
	}

	void Timestep::Status()
	{
		const int movement = MovementFps();

		if (!Active())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Timestep is off, movement follows the frame rate.\n", 0);
		}
		else
		{
			// Measured on the step clock rather than from com_maxfps, so a frame that could not carry the
			// rate shows up here as the rate falling short.
			const int span = Last - First;
			const int reached = span > 0 ? static_cast<int>(1000LL * Emitted / span) : 0;

			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Timestep: com_maxfps {} is a {} ms step, drawing at {} fps, reaching {} steps a second.\n",
					movement, 1000 / movement, RenderFps(), reached)
					.c_str(),
				0);
		}
		if (LastIdeal > 0)
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Last jump peaked {:.2f} above takeoff. A standing jump at com_maxfps {} peaks {:.2f}.\n",
					LastApex, movement, LastIdeal)
					.c_str(),
				0);
		}
	}

	// Taken at CL_FinishMove, which is the last thing the engine does to a command and the first
	// chance a module gets at it, so the log can tell the two apart.
	void Timestep::Sample(const usercmd_s& cmd)
	{
		Raw = cmd;
	}

	// Opens the per pmove step log on demand and closes it when logging stops, so the two files
	// share one dvar. Returns false while there is nothing to write to.
	static bool OpenTrace(std::ofstream& trace, bool logging, bool timestep)
	{
		if (!logging)
		{
			if (trace.is_open())
				trace.close();
			return false;
		}
		if (trace.is_open())
			return true;

		const std::filesystem::path path = Environment::Path(Directory::App) / "Logs";

		std::error_code ec;
		std::filesystem::create_directories(path, ec);

		trace.open(path / (timestep ? "timestep_steps_on.csv" : "timestep_steps_off.csv"), std::ios::trunc);
		if (!trace.is_open())
			return false;

		trace << "kind,commandTime,msec,originZ,speed,velZ,ground,pmFlags,jumpTime,jumpOriginZ,normalZ,walkable,"
				 "delta,active,snap,oldSnap,realtime,newSnaps\n";
		return true;
	}

	// One row per pmove step, taken at the ground trace, which runs first in a step and so reports
	// the state the move is about to be run from.
	void Timestep::Step(const pmove_t* pm, const pml_t* pml)
	{
		// Only the local player's prediction, and each command only the first time: prediction replays
		// the same commands every frame, and a listen server runs every client's moves through here too.
		if (!Enabled || pm->ps != &cgs->predictedPlayerState || pm->cmd.serverTime <= Stepped)
			return;
		Stepped = pm->cmd.serverTime;

		const playerState_s& ps = *pm->ps;
		TrackJump(ps);

		if (!Log || !OpenTrace(Trace, Log->current.enabled, Enabled->current.enabled))
			return;

		// pml->msec is the width pmove really stepped, which is not the spacing of the commands
		// whenever the command clock has drifted from what the playerState was last predicted to.
		Trace << "step," << pm->cmd.serverTime << ',' << pml->msec << ',' << ps.origin[2] << ','
			  << glm::length(vec2(ps.velocity)) << ',' << ps.velocity[2] << ','
			  << (ps.groundEntityNum != ENTITYNUM_NONE ? 1 : 0) << ',' << ps.pm_flags << ',' << ps.jumpTime << ','
			  << ps.jumpOriginZ << ",,," << clients->serverTimeDelta << ',' << (Active() ? 1 : 0) << ','
			  << clients->snap.serverTime << ',' << clients->oldSnapServerTime << ',' << cls->realtime << ','
			  << clients->newSnapshots << '\n';
	}

	// The CoD4 bounce, which turns the speed of a fall into speed along the ground. Written with the
	// surface it fired against, because on flat walkable ground it should never fire at all.
	void Timestep::Bounce(const pmove_t* pm, const pml_t* pml, const trace_t& trace, float before)
	{
		if (!Log || !Trace.is_open())
			return;

		const playerState_s& ps = *pm->ps;

		Trace << "bounce," << pm->cmd.serverTime << ',' << pml->msec << ',' << ps.origin[2] << ',' << before << ','
			  << ps.velocity[2] << ',' << (ps.groundEntityNum != ENTITYNUM_NONE ? 1 : 0) << ',' << ps.pm_flags << ','
			  << ps.jumpTime << ',' << ps.jumpOriginZ << ',' << trace.normal[2] << ',' << (trace.walkable ? 1 : 0)
			  << '\n';
	}

	// Jump height is the plainest readout of the step width there is: Sys_SnapVector rounds the
	// velocity every step, so 333 peaks near 46.4 and 250 near 41.6 where the true height is 39. The
	// state seen here is the one a step starts from, so the highest of them is the peak pmove reached.
	void Timestep::TrackJump(const playerState_s& ps)
	{
		const bool grounded = ps.groundEntityNum != ENTITYNUM_NONE;

		// Rising off the ground with the jump flag up. Walking off a ledge or a ramp does not rise, and
		// the flag outlives a landing in CoD4, so it alone would take either for a jump.
		const bool takeoff = Grounded && !grounded && (ps.pm_flags & PMF_JUMPING) && ps.velocity[2] > 0;
		Grounded = grounded;

		if (takeoff)
		{
			Jumping = true;
			JumpBase = ps.jumpOriginZ;
			JumpTop = ps.origin[2];
			return;
		}
		if (!Jumping)
			return;

		JumpTop = std::max(JumpTop, ps.origin[2]);
		if (!grounded && ps.velocity[2] > 0)
			return;

		Jumping = false;
		LastApex = JumpTop - JumpBase;
		LastIdeal = IdealApex(ps);

		if (Apex && Apex->current.enabled)
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Jump peaked {:.2f}, a standing jump at com_maxfps {} peaks {:.2f}\n", LastApex,
					MovementFps(), LastIdeal)
					.c_str(),
				0);
		}
	}

	// Every mode takes off at sqrt(2 g jump_height) and falls under the same averaged gravity, so one
	// formula covers them all.
	float Timestep::IdealApex(const playerState_s& ps)
	{
		static const auto height = Dvar::Find("jump_height");

		const int movement = MovementFps();
		if (!height || movement <= 0 || ps.gravity <= 0)
			return 0;

		const float gravity = static_cast<float>(ps.gravity);
		const float velocity = std::sqrt(2.0f * gravity * height->current.value);

		return JumpApex(velocity, gravity, 1000 / movement);
	}

	// Height a standing jump peaks at when every pmove step is msec wide. Sys_SnapVector rounds the
	// velocity to a whole unit after every step, so gravity is lost in whole units and the height is
	// a property of the step width rather than of the jump.
	float Timestep::JumpApex(float velocity, float gravity, int msec)
	{
		const float frametime = static_cast<float>(std::max(msec, 1)) * 0.001f;

		float height = 0;
		float peak = 0;

		// PM_SlideMove moves on the mean of the velocity either side of the step's gravity.
		for (int i = 0; i < 100000 && (velocity > 0 || height >= peak); i++)
		{
			const float end = velocity - gravity * frametime;

			height += (velocity + end) * 0.5f * frametime;
			velocity = std::nearbyint(end);
			peak = std::max(peak, height);
		}
		return peak;
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

			Journal << "serverTime,msec,forwardmove,rightmove,buttons,rawForward,rawRight,rawButtons,cmdNumber,"
					   "psCommandTime,yaw,frameTime,sysMsgTime,speed,ground\n";
			Logged = 0;
			Previous = cmd.serverTime;
		}
		Journal << cmd.serverTime << ',' << cmd.serverTime - Previous << ',' << int(cmd.forwardmove) << ','
				<< int(cmd.rightmove) << ',' << cmd.buttons << ',' << int(Raw.forwardmove) << ',' << int(Raw.rightmove)
				<< ',' << Raw.buttons << ',' << clients->cmdNumber << ','
				<< cgs->predictedPlayerState.commandTime << ',' << cmd.angles[1] << ',' << com_frameTime << ','
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
		LastIdeal = 0;
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
			Status();
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
			// Picked up again a frame behind wherever the clock is then, rather than owing the gap.
			Vanilla.Started = false;
			Emitted = 0;
			First = 0;
			Sent = 0;

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

	// A packet holds the newest 32 commands, so any more made between two packets never reach the
	// server, which then steps the whole gap as one wide move. A client at com_maxfps loses them too
	// when cl_maxpackets is low, but here packets only leave on rendered frames, so a low frame rate
	// loses them where the client being imitated would not. Read off the packets already sent, so it
	// is only ever reported once it has happened.
	void Timestep::CheckPackets()
	{
		int sent = 0;
		for (const outPacket_t& packet : clients->outPackets)
			sent = std::max(sent, packet.p_cmdNumber);

		// At most one packet leaves a frame, and this runs every frame, so the gap is one packet's worth.
		const int between = Sent ? sent - Sent : 0;
		Sent = sent;

		if (Dropping || between <= MaxSteps)
			return;

		Dropping = true;

		static const auto maxPackets = Dvar::Find("cl_maxpackets");
		Com_PrintMessage(CON_CHANNEL_ERROR,
			std::format("^1Timestep: {} commands were made between two packets at com_maxfps {} and {} fps, and a "
						"packet carries 32, so the server stepped {} of them as one. Raise cl_maxpackets{} or "
						"sr_maxfps.\n",
				between, MovementFps(), RenderFps(), between - MaxSteps + 1,
				maxPackets ? std::format(" (now {})", maxPackets->current.integer) : "")
				.c_str(),
			0);
	}

	// Com_Frame's client branch (0x500037, and CoD4X's rewrite of it) only waits for the millisecond to
	// tick over, credits the frame max(elapsed, 1000/com_maxfps), and R_WaitEndTime then holds it until
	// wall time catches up. So a client that keeps up steps an exact grid however long its wait takes,
	// and one that cannot sends a single wide command for the frame it lost. The virtual client starts
	// a frame behind the real one, or it would owe nothing on the frame it is seeded and stay a step
	// short for the whole run.
	Steps Timestep::PlanSteps(int* widths, int step, int target, int frametime)
	{
		if (step < 1)
			step = 1;

		if (!Vanilla.Started || std::abs(target - Vanilla.Time) > MaxDrift)
		{
			Vanilla.Time = target - (frametime > 0 ? frametime : step);
			Vanilla.Started = true;
		}

		Steps plan;
		plan.Clock = Vanilla.Time;

		while (plan.Count < MaxSteps && target - Vanilla.Time >= step)
		{
			widths[plan.Count++] = step;
			Vanilla.Time += step;
		}

		// More than a packet can carry. The last command takes the rest, which is the one wide command a
		// client that hitched for this long would have sent, and keeps the clock from falling behind.
		if (plan.Count == MaxSteps && target - Vanilla.Time >= step)
		{
			const int owed = target - Vanilla.Time;
			const int spent = owed - owed % step;

			widths[MaxSteps - 1] += spent;
			Vanilla.Time += spent;
			plan.Starved = true;
		}
		return plan;
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

		CheckPackets();

		int widths[MaxSteps] = {};
		const Steps plan = PlanSteps(widths, step, target, cls->frametime);
		const int count = plan.Count;

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

		if (!Emitted)
		{
			First = plan.Clock;
			Stamp = frameTime - msec;
		}

		int time = plan.Clock;
		for (int i = 1; i <= count; i++)
		{
			const int width = widths[i - 1];
			time += width;

			// Placed on the wall clock the binds stamp, not on realtime's, which falls behind at every
			// clamped hitch and would throw the stance hold's timer off.
			const int stamp = frameTime - (target - time);

			// Both halves of CL_KeyState's ratio come off this one clock, as in CL_Frame, so a held key
			// reads whole even on the step after a hitch or a change of com_maxfps.
			com_frameTime = stamp;
			frame_msec = std::clamp(stamp - Stamp, 1, 200);
			Stamp = stamp;

			clients->serverTime = time + delta;
			cls->frametime = width;

			// The frame's mouse travel belongs to the frame, so hand each step its own share of it.
			clients->mouseDx[clients->mouseIndex] = dx * i / count - dx * (i - 1) / count;
			clients->mouseDy[clients->mouseIndex] = dy * i / count - dy * (i - 1) / count;

			CL_CreateNewCommands_h(localClientNum);
			Record(clients->cmds[clients->cmdNumber & 0x7F]);

			// A client really running at this rate predicts between one command and the next, so
			// anything that reads the predicted state while a command is being built - the bhop
			// deciding whether it is standing on something, the strafe helpers - sees it advance.
			// Left to the engine's one call a frame, every command in a frame is built against the
			// state the frame started in, and a landing lands on the wrong command. The last step
			// is skipped because the engine predicts right after this returns, which keeps the
			// predictions per second the same as the client being imitated pays.
			if (i < count)
				Client::Predict(localClientNum);
		}
		Emitted += count;
		Last = time;

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
