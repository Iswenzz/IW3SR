#include "Sim/Run.hpp"

#include "Engine/Trace.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace Sim
{
	constexpr int PlayerGravity = 800;
	// CL_FinishMove packs angles with this; Client.cpp keeps its own copy at internal linkage.
	constexpr double BotAngle2Short = 182.0444488525391;

	playerState_s Spawn(int speed)
	{
		playerState_s ps = {};

		ps.pm_type = PM_NORMAL;
		ps.speed = speed;
		ps.gravity = PlayerGravity;
		ps.groundEntityNum = ENTITYNUM_NONE;
		ps.viewHeightTarget = 60;
		ps.viewHeightCurrent = 60.0f;
		ps.clientNum = 0;
		// Set by the game, never by pmove. Left at zero it silently zeroes PM_CmdScale_Walk.
		ps.moveSpeedScaleMultiplier = 1.0f;
		ps.sprintState.sprintStartMaxLength = 4000;
		ps.origin = { 0.0f, 0.0f, 0.0f };
		ps.velocity = { 0.0f, 0.0f, 0.0f };
		ps.commandTime = 0;

		return ps;
	}

	// Strafebot::GetOptYawDelta, with the three variants side by side so the harness can say
	// which one the real Pmove rewards. Returns the absolute view yaw to hold, or nothing when
	// the bot declines to aim.
	std::optional<float> BotYaw(Bot bot, Mode mode, const playerState_s& ps, const usercmd_s& cmd, int fps,
		float safety, int& inferred, float hysteresis, bool airControl)
	{
		const float speed = glm::length(vec2(ps.velocity.x, ps.velocity.y));
		if ((!cmd.rightmove && !cmd.forwardmove) || speed < 1.0f)
			return std::nullopt;

		// Only the modal variant knows what to do without a strafe key; the CoD4 form picks its
		// side from the sign of rightmove and has nothing to go on.
		if (!cmd.rightmove && bot != Bot::Modal)
			return std::nullopt;

		// A strafe key alternates and the two beats' turns cancel. Forward alone has no beat,
		// so holding theta to one side flies a circle: net displacement fell 787 to 24 in CS.
		// CPM air control is exempt because turning is the point of holding it.
		const bool cpmTurn = airControl && mode == Mode::Q3CPM && cmd.forwardmove && !cmd.rightmove;
		if (!cmd.rightmove && bot == Bot::Modal && !cpmTurn)
			return std::nullopt;

		const float f = static_cast<float>(fps);
		const float wishspeed = static_cast<float>(ps.speed);

		float accel = f / wishspeed * (635.0f / f);
		if (fps == 333)
			accel = f / wishspeed * (350.0f / f);
		if (fps == 250)
			accel = f / wishspeed * std::pow(f / wishspeed, 2.0f);
		if (fps == 125)
			accel = wishspeed / f * (333.0f / f);

		if (bot == Bot::Scaled)
		{
			const int msec = std::max(1, 1000 / fps);
			accel = safety * (msec / 1000.0f) * std::max(100.0f, wishspeed);
		}

		// Every mode's accelerate() clips the step at SpeedCap - dot(velocity, wishdir), and
		// only the three numbers differ. The CoD4 form above is wrong wherever they do: CS caps
		// what the step may reach at 30 while scaling the step by the uncapped wish speed, and
		// a bare strafe key in CPM swaps in the 30 cap with 70x acceleration.
		float cap = wishspeed;
		if (bot == Bot::Modal)
		{
			const float seconds = std::max(1, 1000 / fps) / 1000.0f;
			float step = seconds * std::max(100.0f, wishspeed);

			switch (mode)
			{
			case Mode::COD4: break;
			case Mode::Q3: step = seconds * wishspeed; break;
			case Mode::Q3CPM:
				if (!cmd.forwardmove && cmd.rightmove)
				{
					cap = std::min(wishspeed, 30.0f);
					step = 70.0f * seconds * cap;
				}
				else
				{
					step = seconds * wishspeed;
				}
				break;
			case Mode::CS:
			{
				const float wish = std::min(wishspeed, 250.0f);
				cap = std::min(wish, 30.0f);
				step = 150.0f * 0.01f * wish; // tick gated at 100 Hz, not per frame
				break;
			}
			}
			// The margin keeps the aim clear of the cliff at cap - step, and a multiple of the
			// step is the right shape while the step is small against the cap: in CoD4 it is
			// 0.57 against 190. A CPM half beat is 16.8 against 30, where the same multiple
			// swallows the whole projection, so it is also held to half the headroom.
			const float headroom = std::max(0.0f, cap - step);
			accel = step + std::min((safety - 1.0f) * step, 0.5f * headroom);
		}

		constexpr float Deg = 3.14159265358979323846f / 180.0f;

		// CPM with forward alone is not an acceleration technique. Q3::AirControl renormalises
		// the velocity to the speed it came in with, so the step is a pure turn, and that is
		// what defrag uses it for: swinging the heading onto a new line without paying for it.
		// The turn is atan2(k sin t, speed + k cos t) with k proportional to cos(t)^2, which
		// peaks at atan(1/sqrt 2) once the step is small against the speed.
		float airTheta = 0.0f;
		if (cpmTurn && bot == Bot::Modal)
		{
			const float seconds = std::max(1, 1000 / fps) / 1000.0f;
			float best = -1.0f;

			for (float t = 1.0f; t < 89.0f; t += 0.25f)
			{
				const float dot = std::cos(t * Deg);
				const float k = 32.0f * 150.0f * dot * dot * seconds * 1.0f;
				const float turn = std::atan2(k * std::sin(t * Deg), speed + k * dot);
				if (turn > best)
				{
					best = turn;
					airTheta = t;
				}
			}
		}

		// The projection cannot go below zero: once the step is larger than the cap itself, as
		// CS's 285 against a cap of 30 is, the best available angle is a right angle to the
		// velocity, not a reflex one. Without the floor the aim swings past 90 and pushes back
		// along the velocity, which is what a negative projection means.
		const float raw = std::max(0.0f, cap - accel) / speed;
		// The original returns 0 here, which the caller reads as "no aim at all".
		if (bot == Bot::Original && (raw > 1.0f || raw < -1.0f))
			return std::nullopt;

		constexpr float Rad = 180.0f / 3.14159265358979323846f;
		const float angle = airTheta > 0.0f ? airTheta : std::acos(std::clamp(raw, -1.0f, 1.0f)) * Rad;
		const float velocityYaw = std::atan2(ps.velocity.y, ps.velocity.x) * Rad;
		const float wishOffset
			= std::atan2(static_cast<float>(-cmd.rightmove), static_cast<float>(cmd.forwardmove)) * Rad;

		int side = cmd.rightmove > 0 ? 1 : cmd.rightmove < 0 ? -1 : 0;
		if (side)
		{
			inferred = side;
		}
		else
		{
			// Forward alone: no key says which way to lean, so carry on the way the view
			// already leans off the velocity. Hysteresis because the bot owns the yaw it is
			// reading that lean from, so a bare sign test chatters across the crossing.
			float lean = ps.viewangles[1] + wishOffset - velocityYaw;
			lean = std::fmod(lean + 540.0f, 360.0f) - 180.0f;

			if (!inferred || std::fabs(lean) > hysteresis)
				inferred = lean >= 0.0f ? -1 : 1;
			side = inferred;
		}
		return velocityYaw - wishOffset - side * angle;
	}

	// AutoFPS' ring model: each rate owns out to g_speed * frame_ms / divisor degrees off the
	// beat's alignment, ordered by frame duration because a longer step stops being clipped
	// further from the velocity. Returns the rate that owns the angle currently being held.
	int RingFps(const playerState_s& ps, const usercmd_s& cmd, float divisor, Mode mode, bool cpmHalf)
	{
		constexpr float Rad = 180.0f / 3.14159265358979323846f;
		constexpr int Ladder[] = { 3, 4, 5, 8 };

		const float speed = glm::length(vec2(ps.velocity.x, ps.velocity.y));
		if (speed < 1.0f || (!cmd.rightmove && !cmd.forwardmove))
			return 0;

		const float velocityYaw = std::atan2(ps.velocity.y, ps.velocity.x) * Rad;
		const float wishOffset
			= std::atan2(static_cast<float>(-cmd.rightmove), static_cast<float>(cmd.forwardmove)) * Rad;

		// A CPM half beat caps its step at 30 regardless of angle, so the angle cannot pick
		// its rate - a shorter frame simply leaves less of the cap unspent.
		if (cpmHalf && mode == Mode::Q3CPM && !cmd.forwardmove && cmd.rightmove)
			return 1000;

		float theta = ps.viewangles[1] + wishOffset - velocityYaw;
		theta = std::fabs(std::fmod(theta + 540.0f, 360.0f) - 180.0f);

		for (const int msec : Ladder)
			if (theta < static_cast<float>(ps.speed) * msec / divisor)
				return 1000 / msec;

		return 1000 / Ladder[std::size(Ladder) - 1];
	}

	int RateAt(const RunConfig& config, int time)
	{
		if (config.rates.empty())
			return config.comMaxFps;

		int cycle = 0;
		for (const RateLeg& leg : config.rates)
			cycle += leg.durationMs;

		if (cycle <= 0)
			return config.comMaxFps;

		int offset = time % cycle;
		for (const RateLeg& leg : config.rates)
		{
			if (offset < leg.durationMs)
				return leg.fps;
			offset -= leg.durationMs;
		}
		return config.rates.back().fps;
	}

	std::vector<RateLeg> ParseRates(const std::string& text)
	{
		std::vector<RateLeg> legs;
		std::stringstream stream(text);
		std::string item;

		while (std::getline(stream, item, ','))
		{
			const size_t colon = item.find(':');
			if (colon == std::string::npos)
				throw std::runtime_error("rate legs look like 333:1000,125:1000 - got " + item);

			const int fps = std::stoi(item.substr(0, colon));
			const int duration = std::stoi(item.substr(colon + 1));

			if (fps <= 0 || duration <= 0)
				throw std::runtime_error("a rate leg needs a positive fps and duration: " + item);
			legs.push_back({ fps, duration });
		}
		return legs;
	}

	static void ApplyKey(ClientState& cl, const KeyEvent& event, int now)
	{
		kbutton_t* key = &cl.kb[event.key];

		if (event.down)
			KeyDown(cl, key, 1, now);
		else
			KeyUp(cl, key, 1, now);
	}

	RunResult Run(const RunConfig& config)
	{
		World world = World::Flat();
		SetWorld(&world);
		SetMode(config.mode);

		ClientState cl;
		cl.cl_sensitivity = config.sensitivity;
		// A server that has been up a while. Starting server time at zero would make every
		// "time since" test in pmove, the 500 ms jump cooldown above all, read as freshly expired.
		cl.serverTimeDelta = 500000;

		HandConfig handConfig = config.hand;
		handConfig.durationMs = config.durationMs;

		Hand hand(handConfig);
		hand.Calibrate(cl.cl_sensitivity, cl.m_yaw);

		playerState_s ps = Spawn(config.gSpeed);
		RunResult result;

		Clock clock;
		SplitStats stats;
		IW3SR::Cadence cadence;

		int lastFrameTime = 0;
		int movementClock = 0;
		int pumpedTo = 0;
		int previousTime = 0;
		vec2 previousOrigin = {};
		float previousHeading = -1000.0f;
		size_t nextEvent = 0;
		bool seeded = false;
		char lastStrafe = 0;
		int inferredSide = 0;
		int airborneCommands = 0;
		int lastStrafeTime = 0;

		while (clock.Milliseconds() < config.durationMs)
		{
			int movementFps = RateAt(config, clock.Milliseconds());

			// The game writes com_maxfps from OnFinishMove, so the rate a frame runs at is the
			// one the previous command's angle chose.
			if (config.autoFps && ps.groundEntityNum == ENTITYNUM_NONE)
			{
				const int chosen = RingFps(ps, cl.cmds[cl.cmdNumber & 0x7F], config.ringDivisor, config.mode, config.cpmHalfBoost);
				if (chosen > 0)
					movementFps = chosen;
			}

			// One bit, read straight off the command: a half beat and a full beat want
			// different rates, and no angle model can see which one is being held.
			// What each technique wants differs by mode, which one pair cannot express: a CPM
			// half beat caps its step at 30, so a shorter frame leaves less of the cap unspent,
			// while CS is flat because its air accel is gated to a 100 Hz tick, not to frames.
			if (config.autoFpsTechnique && ps.groundEntityNum == ENTITYNUM_NONE)
			{
				const bool half = cl.cmds[cl.cmdNumber & 0x7F].forwardmove == 0;
				switch (config.mode)
				{
				case Mode::COD4: movementFps = 250; break;
				case Mode::Q3: movementFps = 250; break;
				case Mode::Q3CPM: movementFps = half ? 1000 : 250; break;
				case Mode::CS: movementFps = 333; break;
				}
			}
			const int renderFps = config.timestep && config.srMaxFps > 0 ? config.srMaxFps : movementFps;

			const int msec = Com_ModifyMsec(
				ComFrameLimiter(clock, cl.com_frameTime, lastFrameTime, renderFps, config.pacing));
			const int now = clock.Milliseconds();

			// Com_EventLoop: binds run and raw mouse counts land in the buffer, both before any
			// command is built, which is why a key can only ever latch on a frame boundary.
			while (nextEvent < hand.Events().size() && hand.Events()[nextEvent].time <= now)
			{
				ApplyKey(cl, hand.Events()[nextEvent], now);
				nextEvent++;
			}
			cl.mouseDx[cl.mouseIndex] += hand.YawCountsBetween(pumpedTo, now);
			pumpedTo = now;

			CL_RunOncePerClientFrame(cl, msec);
			cl.serverTime = cl.realtime + cl.serverTimeDelta;

			const int before = cl.cmdNumber;

			if (config.timestep)
			{
				SplitCommands(cl, movementClock, movementFps, config.split, cadence, config.pacing, stats);
			}
			else
			{
				CL_CreateNewCommands(cl);
			}

			// The bot rewrites the command's angles after it is built and before pmove runs it,
			// which is exactly where Strafebot::OnFinishMove sits in the game - including its
			// early return on the ground. Aiming an air solution at a grounded command is not
			// something the plugin ever does, and leaving it out of the harness made ground
			// heavy scenarios read as regressions that do not exist.
			if (config.bot != Bot::None && ps.groundEntityNum == ENTITYNUM_NONE)
			{
				for (int number = before + 1; number <= cl.cmdNumber; number++)
				{
					usercmd_s& cmd = cl.cmds[number & 0x7F];

					// DisableStrafeDowntime: hold the last strafe key across the gap between
					// beats so there is still a wish direction to accelerate along.
					if (config.downtimeMs > 0 && ps.groundEntityNum == ENTITYNUM_NONE)
					{
						if (cmd.rightmove)
						{
							lastStrafe = cmd.rightmove;
							lastStrafeTime = cmd.serverTime;
						}
						else if (lastStrafe && cmd.serverTime - lastStrafeTime < config.downtimeMs)
						{
							cmd.rightmove = lastStrafe;
						}
					}

					const std::optional<float> target
						= BotYaw(config.bot, config.mode, ps, cmd, movementFps, config.safetyFactor, inferredSide,
							config.sideHysteresis, config.airControl);

					if (!target)
					{
						result.botIdle++;
						continue;
					}
					float written = *target;

					// Ease onto the target over the first commands of a jump instead of snapping
					// the whole way on the first one. Confined to the engage: rate limiting every
					// command sweeps through the dead angle at each beat change and costs far more
					// than it buys.
					if (config.engageRamp > 0 && airborneCommands < config.engageRamp)
					{
						const float blend = static_cast<float>(airborneCommands + 1) / config.engageRamp;
						float delta = *target - cl.viewangles[1];
						delta = std::fmod(delta + 540.0f, 360.0f) - 180.0f;
						written = cl.viewangles[1] + delta * blend;
					}
					airborneCommands++;

					cl.viewangles[1] = written;
					cmd.angles[1] = static_cast<uint16_t>(static_cast<int>(written * BotAngle2Short));
				}
			}

			for (int number = before + 1; number <= cl.cmdNumber; number++)
			{
				const usercmd_s& cmd = cl.cmds[number & 0x7F];
				const usercmd_s& oldcmd = cl.cmds[(number - 1) & 0x7F];

				// The first command has nothing behind it to measure a step against, so it seeds
				// the physics clock rather than being run as a move of its own.
				if (!seeded)
				{
					ps.commandTime = cmd.serverTime;
					previousTime = cmd.serverTime;
					previousOrigin = vec2(ps.origin.x, ps.origin.y);
					seeded = true;
					continue;
				}
				const bool wasGrounded = ps.groundEntityNum != ENTITYNUM_NONE;

				pmove_t pm = MakePmove(&ps, cmd, oldcmd);
				Pmove(&pm);

				const bool grounded = ps.groundEntityNum != ENTITYNUM_NONE;
				if (wasGrounded && !grounded)
					result.jumps++;
				if (grounded)
					airborneCommands = 0;
				if (grounded)
					result.groundCommands++;

				Sample sample = {};
				sample.serverTime = cmd.serverTime;
				sample.msec = cmd.serverTime - previousTime;
				sample.frame = result.frames;
				sample.movementFps = movementFps;
				sample.buttons = cmd.buttons;
				sample.forwardmove = cmd.forwardmove;
				sample.rightmove = cmd.rightmove;
				sample.yaw = SHORT2ANGLE(cmd.angles[1]);
				sample.origin = ps.origin;
				sample.velocity = ps.velocity;
				sample.speed = glm::length(vec2(ps.velocity.x, ps.velocity.y));
				sample.onGround = grounded;

				result.peakSpeed = std::max(result.peakSpeed, sample.speed);
				if (sample.speed > 1.0f && previousHeading > -720.0f)
				{
					constexpr float Rad = 180.0f / 3.14159265358979323846f;
					const float heading = std::atan2(ps.velocity.y, ps.velocity.x) * Rad;
					float swing = std::fmod(heading - previousHeading + 540.0f, 360.0f) - 180.0f;
					result.turned += std::fabs(swing);
					previousHeading = heading;
				}
				else if (sample.speed > 1.0f)
				{
					constexpr float Rad = 180.0f / 3.14159265358979323846f;
					previousHeading = std::atan2(ps.velocity.y, ps.velocity.x) * Rad;
				}
				result.msecHistogram[sample.msec]++;
				result.physicsMs += sample.msec;
				result.distance += glm::length(vec2(ps.origin.x, ps.origin.y) - previousOrigin);
				result.displacement = glm::length(vec2(ps.origin.x, ps.origin.y));
				result.samples.push_back(sample);

				previousOrigin = vec2(ps.origin.x, ps.origin.y);
				previousTime = cmd.serverTime;
				result.commands++;
			}
			result.frames++;
		}
		result.starvedFrames = stats.starved;
		result.finalSpeed = result.samples.empty() ? 0.0f : result.samples.back().speed;

		double total = 0.0;
		for (const Sample& sample : result.samples)
			total += sample.speed;
		result.meanSpeed = result.samples.empty() ? 0.0f : static_cast<float>(total / result.samples.size());

		SetWorld(nullptr);
		return result;
	}

	const char* ModeName(Mode mode)
	{
		switch (mode)
		{
		case Mode::COD4:
			return "cod4";
		case Mode::Q3:
			return "q3";
		case Mode::Q3CPM:
			return "q3cpm";
		case Mode::CS:
			return "cs";
		}
		return "?";
	}

	Mode ParseMode(const std::string& name)
	{
		if (name == "cod4")
			return Mode::COD4;
		if (name == "q3")
			return Mode::Q3;
		if (name == "q3cpm")
			return Mode::Q3CPM;
		if (name == "cs")
			return Mode::CS;
		throw std::runtime_error("unknown mode: " + name);
	}
}
