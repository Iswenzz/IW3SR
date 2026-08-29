#pragma once
// One client, run for a fixed span of virtual time against one hand, with every command it
// produced and the state each command left behind.
#include "Engine/Client.hpp"
#include "Engine/Com.hpp"
#include "Engine/Pmove.hpp"
#include "Sim/Hand.hpp"
#include "Sim/Timestep.hpp"

#include <map>
#include <string>
#include <vector>

namespace Sim
{
	// One leg of a com_maxfps schedule. A run that never changes rate is a single leg.
	struct RateLeg
	{
		int fps;
		int durationMs;
	};

	enum class Bot
	{
		None,
		Original,
		Clamped,
		Scaled,
		Modal,
	};

	struct RunConfig
	{
		Mode mode = Mode::Q3CPM;
		bool timestep = false;
		int comMaxFps = 333;
		// What the renderer is capped at with the timestep on. Zero follows com_maxfps, which is
		// what the mod does when sr_maxfps is unset.
		int srMaxFps = 0;
		// Empty means com_maxfps holds for the whole run. Otherwise the legs cycle.
		std::vector<RateLeg> rates;

		int durationMs = 3000;
		HandConfig hand;
		float sensitivity = 5.0f;
		// What the game does. Grid is the even division it used to do, kept only for contrast.
		SplitMode split = SplitMode::Limiter;
		// g_speed. 190 is stock CoD4, 210 is what most speedrun servers run.
		int gSpeed = 190;
		Pacing pacing;
		// Drives the view from Strafebot::GetOptYawDelta instead of the hand, so the three
		// variants can be compared against the real Pmove rather than against a model of it.
		Bot bot = Bot::None;
		float safetyFactor = 3.232f;
		// CPM forward-only aims for maximum turn rate rather than acceleration.
		bool airControl = true;
		// Fold the only measured technique win into the angle path instead of a separate mode.
		bool cpmHalfBoost = false;
		// Picks com_maxfps from the strafe angle each frame, the way AutoFPS does, so the
		// zone model can be judged against a fixed rate instead of argued about.
		bool autoFps = false;
		// Picks com_maxfps from which technique is held rather than from the strafe angle:
		// forwardmove == 0 is a half beat, which wants a different rate to a full beat.
		bool autoFpsTechnique = false;
		float ringDivisor = 20.0f;
		// Milliseconds the last strafe key is held across a beat gap. Zero is off.
		int downtimeMs = 0;
		// Degrees the lean must pass before an inferred side flips. Zero is a bare sign test.
		float sideHysteresis = 0.0f;
		// Commands after leaving the ground over which the aim eases onto its target. The bot
		// is idle while grounded, so without this it snaps the whole way on the first airborne
		// command - up to 90 degrees in CS.
		int engageRamp = 0;
	};

	struct Sample
	{
		int serverTime;
		int msec;
		int frame;
		int movementFps;
		int buttons;
		int forwardmove;
		int rightmove;
		float yaw;
		vec3 origin;
		vec3 velocity;
		float speed;
		bool onGround;
	};

	struct RunResult
	{
		std::vector<Sample> samples;
		std::map<int, int> msecHistogram;
		int frames = 0;
		int commands = 0;
		int starvedFrames = 0;
		int jumps = 0;
		int groundCommands = 0;
		float peakSpeed = 0.0f;
		float finalSpeed = 0.0f;
		float meanSpeed = 0.0f;
		float distance = 0.0f;
		// Total physics milliseconds the commands actually carried, which should equal the run.
		int physicsMs = 0;
		// Commands where the bot declined to aim. The original bails whenever acos would
		// be NaN, which is every command below wishspeed - accel.
		int botIdle = 0;
		// Total heading swing, for judging a turning technique rather than an accelerating one.
		float turned = 0.0f;
		// Straight-line displacement from spawn. distance is path length, which a bot that
		// curves you inflates without getting you anywhere.
		float displacement = 0.0f;
	};

	RunResult Run(const RunConfig& config);

	playerState_s Spawn(int speed);

	// com_maxfps in force at a point in the run.
	int RateAt(const RunConfig& config, int time);

	const char* ModeName(Mode mode);
	Mode ParseMode(const std::string& name);
	std::vector<RateLeg> ParseRates(const std::string& text);
}
