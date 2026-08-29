// Runs the game's movement offline: the real command builder, the real frame limiter, the real
// physics, and the mod's command splitting, with nothing between them and a virtual clock. Two
// clients are given the same hand and compared on what pmove was actually handed.
#include "Sim/Run.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace Sim;

namespace
{
	struct Options
	{
		RunConfig config;
		std::string csv;
		bool compare = false;
		bool sweep = false;
		bool matrix = false;
		bool selfTest = false;
	};

	// Every movement key combination worth a row. Backwards combinations are in because
	// PM_CmdScale_Walk scales them differently and the mod's Q3 path does not.
	const char* const Combos[] = { "w", "a", "d", "s", "wa", "wd", "sa", "sd", "ad" };

	void Usage()
	{
		std::cout <<
			"timestep-sim - offline comparison of vanilla movement against the timestep\n\n"
			"  --mode <cod4|q3|q3cpm|cs>   physics mode (default q3cpm)\n"
			"  --g-speed <n>               190 stock, 210 speedrun (default 190)\n"
			"  --keys <wasd letters>       movement keys held, e.g. wa, wd, a, d, ad\n"
			"  --jump <hold|tap|none>      default hold\n"
			"  --jump-period <ms>          tap period (default 400)\n"
			"  --alternate <ms>            swap strafe key and sweep direction on this period\n"
			"  --yaw-rate <deg/s>          mouse sweep, positive turns left\n"
			"  --com-maxfps <n>            movement rate (default 333)\n"
			"  --sr-maxfps <n>             render cap with the timestep on, 0 follows com_maxfps\n"
			"  --rates <fps:ms,...>        cycle com_maxfps through these legs, e.g. 333:2000,125:2000\n"
			"  --duration <ms>             default 3000\n"
			"  --work-us <n>               what a frame's own work costs (default 500)\n"
			"  --sleep-us <n>              what NET_Sleep(1) costs (default 1200)\n"
			"  --split <grid|limiter>      how the split picks step widths (default grid)\n"
			"  --timestep                  run the split client instead of the vanilla one\n"
			"  --compare                   run both clients and print the difference\n"
			"  --sweep                     compare across a grid of rates\n"
			"  --matrix                    every mode, g_speed and key combination\n"
			"  --self-test                 assert the split is identical when render == movement\n"
			"  --csv <path>                write every command to a file\n";
	}

	int Int(const char* text) { return std::atoi(text); }
	float Float(const char* text) { return static_cast<float>(std::atof(text)); }

	bool ParseArgs(int argc, char** argv, Options& options)
	{
		options.config.pacing.sleepMicros = 1200;

		for (int i = 1; i < argc; i++)
		{
			const std::string arg = argv[i];
			const bool hasNext = i + 1 < argc;

			if (arg == "--help" || arg == "-h")
				return false;
			else if (arg == "--mode" && hasNext)
				options.config.mode = ParseMode(argv[++i]);
			else if (arg == "--g-speed" && hasNext)
				options.config.gSpeed = Int(argv[++i]);
			else if (arg == "--keys" && hasNext)
				options.config.hand.keys = Keys::Parse(argv[++i]);
			else if (arg == "--jump" && hasNext)
			{
				const std::string value = argv[++i];
				options.config.hand.jump = value == "none" ? Jump::None
					: value == "tap"                       ? Jump::Tapped
														   : Jump::Held;
			}
			else if (arg == "--bot" && hasNext)
			{
				const std::string value = argv[++i];
				options.config.bot = value == "original" ? Bot::Original
					: value == "clamped"                 ? Bot::Clamped
					: value == "scaled"                  ? Bot::Scaled
					: value == "modal"                   ? Bot::Modal
														 : Bot::None;
			}
			else if (arg == "--engage-ramp" && hasNext)
				options.config.engageRamp = Int(argv[++i]);
			else if (arg == "--hysteresis" && hasNext)
				options.config.sideHysteresis = Float(argv[++i]);
			else if (arg == "--downtime" && hasNext)
				options.config.downtimeMs = Int(argv[++i]);
			else if (arg == "--cpm-half-boost")
				options.config.cpmHalfBoost = true;
			else if (arg == "--no-aircontrol")
				options.config.airControl = false;
			else if (arg == "--autofps-technique")
				options.config.autoFpsTechnique = true;
			else if (arg == "--autofps")
				options.config.autoFps = true;
			else if (arg == "--ring-divisor" && hasNext)
				options.config.ringDivisor = Float(argv[++i]);
			else if (arg == "--safety" && hasNext)
				options.config.safetyFactor = Float(argv[++i]);
			else if (arg == "--jump-period" && hasNext)
				options.config.hand.jumpPeriodMs = Int(argv[++i]);
			else if (arg == "--technique-period" && hasNext)
				options.config.hand.techniquePeriodMs = Int(argv[++i]);
			else if (arg == "--beat-gap" && hasNext)
				options.config.hand.beatGapMs = Int(argv[++i]);
			else if (arg == "--alternate" && hasNext)
				options.config.hand.alternateMs = Int(argv[++i]);
			else if (arg == "--yaw-rate" && hasNext)
				options.config.hand.yawRate = Float(argv[++i]);
			else if (arg == "--com-maxfps" && hasNext)
				options.config.comMaxFps = Int(argv[++i]);
			else if (arg == "--sr-maxfps" && hasNext)
				options.config.srMaxFps = Int(argv[++i]);
			else if (arg == "--rates" && hasNext)
				options.config.rates = ParseRates(argv[++i]);
			else if (arg == "--duration" && hasNext)
				options.config.durationMs = Int(argv[++i]);
			else if (arg == "--work-us" && hasNext)
				options.config.pacing.workMicros = Int(argv[++i]);
			else if (arg == "--sleep-us" && hasNext)
				options.config.pacing.sleepMicros = Int(argv[++i]);
			else if (arg == "--split" && hasNext)
				options.config.split = std::string(argv[++i]) == "limiter" ? SplitMode::Limiter : SplitMode::Grid;
			else if (arg == "--timestep")
				options.config.timestep = true;
			else if (arg == "--compare")
				options.compare = true;
			else if (arg == "--sweep")
				options.sweep = true;
			else if (arg == "--matrix")
				options.matrix = true;
			else if (arg == "--self-test")
				options.selfTest = true;
			else if (arg == "--csv" && hasNext)
				options.csv = argv[++i];
			else
			{
				std::cerr << "unknown argument: " << arg << "\n";
				return false;
			}
		}
		return true;
	}

	// How hard the aim moves between commands. A bot that jitters around its target and one
	// that snaps across on a beat change both "feel choppy" but want opposite fixes, and the
	// distribution of this tells them apart: jitter is a fat median, a snap is a fat tail.
	std::string AimMotion(const RunResult& result)
	{
		std::vector<float> deltas;
		for (size_t i = 1; i < result.samples.size(); i++)
		{
			if (!result.samples[i].onGround && !result.samples[i - 1].onGround)
			{
				float delta = result.samples[i].yaw - result.samples[i - 1].yaw;
				delta = std::fmod(delta + 540.0f, 360.0f) - 180.0f;
				deltas.push_back(std::fabs(delta));
			}
		}
		if (deltas.size() < 4)
			return "no airborne samples";

		std::sort(deltas.begin(), deltas.end());
		const auto at = [&](double q) { return deltas[static_cast<size_t>(q * (deltas.size() - 1))]; };
		const size_t big = std::count_if(deltas.begin(), deltas.end(), [](float d) { return d > 20.0f; });

		std::ostringstream out;
		out << std::fixed << std::setprecision(2) << "median " << at(0.5) << "  p95 " << at(0.95) << "  max "
			<< deltas.back() << "  over20deg " << big << "/" << deltas.size();
		return out.str();
	}

	std::string Histogram(const RunResult& result)
	{
		std::ostringstream out;
		for (const auto& entry : result.msecHistogram)
			out << entry.first << "ms x" << entry.second << "  ";
		return out.str();
	}

	void Print(const char* label, const RunResult& result)
	{
		std::cout << std::fixed << std::setprecision(1);
		std::cout << "  " << std::left << std::setw(10) << label
				  << " cmds " << std::setw(6) << result.commands
				  << " frames " << std::setw(6) << result.frames
				  << " peak " << std::setw(8) << result.peakSpeed
				  << " mean " << std::setw(8) << result.meanSpeed
				  << " dist " << std::setw(9) << result.distance
				  << " net " << std::setw(8) << static_cast<int>(result.displacement)
				  << " turn " << std::setw(7) << static_cast<int>(result.turned)
				  << " jumps " << std::setw(4) << result.jumps
				  << " ground " << std::setw(5) << result.groundCommands
				  << " physics " << result.physicsMs << "ms\n";
		std::cout << "  " << std::setw(10) << " " << " steps: " << Histogram(result) << "\n";
		std::cout << "  " << std::setw(10) << " " << " aim:   " << AimMotion(result) << "\n";
	}

	void WriteCsv(const std::string& path, const RunResult& result)
	{
		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open())
		{
			std::cerr << "could not open " << path << "\n";
			return;
		}
		file << "serverTime,msec,frame,fps,buttons,forwardmove,rightmove,yaw,x,y,z,vx,vy,vz,speed,ground\n";
		for (const Sample& s : result.samples)
		{
			file << s.serverTime << ',' << s.msec << ',' << s.frame << ',' << s.movementFps << ','
				 << s.buttons << ',' << s.forwardmove << ',' << s.rightmove << ',' << s.yaw << ','
				 << s.origin.x << ',' << s.origin.y << ',' << s.origin.z << ',' << s.velocity.x << ','
				 << s.velocity.y << ',' << s.velocity.z << ',' << s.speed << ',' << (s.onGround ? 1 : 0)
				 << '\n';
		}
		std::cout << "wrote " << result.samples.size() << " commands to " << path << "\n";
	}

	float Percent(float from, float to)
	{
		return from == 0.0f ? 0.0f : (to - from) / from * 100.0f;
	}

	// A strafe only gains inside a narrow band of sweep rates, and the band moves with the mode and
	// the rate. Comparing two clients at one arbitrary rate measures the rate, not the clients, so
	// every comparison here is made at each client's own best.
	struct Best
	{
		float yawRate = 0.0f;
		float peakSpeed = 0.0f;
		float distance = 0.0f;
	};

	Best FindBest(RunConfig config)
	{
		Best best;
		for (int rate = 0; rate <= 900; rate += 15)
		{
			config.hand.yawRate = static_cast<float>(rate);
			const RunResult result = Run(config);

			if (result.peakSpeed > best.peakSpeed)
			{
				best.yawRate = config.hand.yawRate;
				best.peakSpeed = result.peakSpeed;
				best.distance = result.distance;
			}
		}
		return best;
	}

	void Compare(RunConfig config)
	{
		RunConfig vanilla = config;
		vanilla.timestep = false;
		vanilla.srMaxFps = 0;

		RunConfig timestep = config;
		timestep.timestep = true;

		const RunResult a = Run(vanilla);
		const RunResult b = Run(timestep);

		std::cout << ModeName(config.mode) << " g_speed " << config.gSpeed
				  << " - com_maxfps " << config.comMaxFps << ", drawn at "
				  << (timestep.srMaxFps ? timestep.srMaxFps : config.comMaxFps) << "\n";
		Print("vanilla", a);
		Print("timestep", b);
		std::cout << std::fixed << std::setprecision(2)
				  << "  -> peak " << Percent(a.peakSpeed, b.peakSpeed) << "%"
				  << ", mean " << Percent(a.meanSpeed, b.meanSpeed) << "%"
				  << ", distance " << Percent(a.distance, b.distance) << "%"
				  << ", commands " << Percent(static_cast<float>(a.commands), static_cast<float>(b.commands))
				  << "%\n\n";
	}

	// The one case the mod is meant to be exactly vanilla: a split that lands on the rate the
	// renderer already runs at has nothing to split, so any difference here at all is a defect.
	int SelfTest(RunConfig base)
	{
		int failures = 0;
		int checks = 0;
		const int rates[] = { 125, 250, 333, 500 };
		const Mode modes[] = { Mode::COD4, Mode::Q3, Mode::Q3CPM, Mode::CS };
		const int speeds[] = { 190, 210 };

		std::cout << "render rate == movement rate, so the split has nothing to split.\n"
				  << "sleep " << base.pacing.sleepMicros << "us, split "
				  << (base.split == SplitMode::Limiter ? "limiter" : "grid") << "\n\n";

		for (Mode mode : modes)
		{
			for (int speed : speeds)
			{
				for (const char* combo : Combos)
				{
					for (int rate : rates)
					{
						RunConfig config = base;
						config.mode = mode;
						config.gSpeed = speed;
						config.hand.keys = Keys::Parse(combo);
						config.hand.yawRate = 200.0f;
						config.hand.alternateMs = 300;
						config.comMaxFps = rate;
						config.durationMs = 2000;
						config.rates.clear();
						config.timestep = false;
						config.srMaxFps = 0;

						RunConfig split = config;
						split.timestep = true;
						split.srMaxFps = rate;

						const RunResult a = Run(config);
						const RunResult b = Run(split);

						const bool same = a.commands == b.commands
							&& std::abs(a.peakSpeed - b.peakSpeed) < 0.01f
							&& std::abs(a.distance - b.distance) < 0.01f;

						checks++;
						if (!same)
						{
							failures++;
							std::cout << "  FAIL " << std::setw(6) << ModeName(mode) << " g" << speed
									  << " " << std::setw(3) << combo << " @" << std::setw(4) << rate
									  << ": cmds " << a.commands << " vs " << b.commands
									  << ", peak " << std::fixed << std::setprecision(2) << a.peakSpeed
									  << " vs " << b.peakSpeed
									  << " (" << std::showpos << Percent(a.peakSpeed, b.peakSpeed)
									  << "%)" << std::noshowpos << "\n";
						}
					}
				}
			}
		}
		// com_maxfps changing under a running client is where the split's carried clock and its
		// step width stop agreeing, so the schedules are checked separately from the fixed rates.
		const char* const schedules[] = {
			"333:500,125:500",
			"125:400,333:400,250:400",
			"250:1000,1000:1000",
			"333:250,76:250",
			"1000:300,125:300,333:300,250:300",
		};

		std::cout << "\nswitching com_maxfps mid run, render still following it:\n\n";

		for (Mode mode : modes)
		{
			for (int speed : speeds)
			{
				for (const char* combo : Combos)
				{
					for (const char* schedule : schedules)
					{
						RunConfig config = base;
						config.mode = mode;
						config.gSpeed = speed;
						config.hand.keys = Keys::Parse(combo);
						config.hand.yawRate = 200.0f;
						config.hand.alternateMs = 300;
						config.rates = ParseRates(schedule);
						config.durationMs = 3000;
						config.timestep = false;
						config.srMaxFps = 0;

						RunConfig split = config;
						split.timestep = true;

						const RunResult a = Run(config);
						const RunResult b = Run(split);

						const bool same = a.commands == b.commands
							&& std::abs(a.peakSpeed - b.peakSpeed) < 0.01f
							&& std::abs(a.distance - b.distance) < 0.01f;

						checks++;
						if (!same)
						{
							failures++;
							std::cout << "  FAIL " << std::setw(6) << ModeName(mode) << " g" << speed
									  << " " << std::setw(3) << combo << " " << std::setw(30) << schedule
									  << ": cmds " << a.commands << " vs " << b.commands
									  << ", peak " << std::fixed << std::setprecision(2) << a.peakSpeed
									  << " vs " << b.peakSpeed
									  << " (" << std::showpos << Percent(a.peakSpeed, b.peakSpeed)
									  << "%)" << std::noshowpos << "\n";
						}
					}
				}
			}
		}

		std::cout << "\n" << (checks - failures) << "/" << checks << " identical, " << failures
				  << " differ\n";
		return failures ? 1 : 0;
	}

	// The question the tool exists for: can the split reach a speed a vanilla client at the same
	// com_maxfps cannot? Each client is measured at its own best sweep rate.
	void Matrix(RunConfig base)
	{
		const Mode modes[] = { Mode::COD4, Mode::Q3, Mode::Q3CPM, Mode::CS };
		const int speeds[] = { 190, 210 };

		std::cout << "peak speed at each client's own best sweep rate. com_maxfps "
				  << base.comMaxFps << ", timestep drawn at "
				  << (base.srMaxFps ? base.srMaxFps : base.comMaxFps) << ", sleep "
				  << base.pacing.sleepMicros << "us, split "
				  << (base.split == SplitMode::Limiter ? "limiter" : "grid") << "\n\n";

		std::cout << std::left << std::setw(7) << "mode" << std::setw(8) << "g_speed"
				  << std::setw(6) << "keys" << std::right << std::setw(10) << "vanilla"
				  << std::setw(10) << "timestep" << std::setw(10) << "peak" << std::setw(12) << "distance"
				  << "\n";

		for (Mode mode : modes)
		{
			for (int speed : speeds)
			{
				for (const char* combo : Combos)
				{
					RunConfig config = base;
					config.mode = mode;
					config.gSpeed = speed;
					config.hand.keys = Keys::Parse(combo);
					config.hand.alternateMs = 300;

					RunConfig vanilla = config;
					vanilla.timestep = false;
					vanilla.srMaxFps = 0;

					RunConfig split = config;
					split.timestep = true;

					const Best a = FindBest(vanilla);
					const Best b = FindBest(split);

					std::cout << std::left << std::setw(7) << ModeName(mode) << std::setw(8) << speed
							  << std::setw(6) << combo << std::right << std::fixed << std::setprecision(1)
							  << std::setw(10) << a.peakSpeed << std::setw(10) << b.peakSpeed
							  << std::setw(9) << std::showpos << std::setprecision(1)
							  << Percent(a.peakSpeed, b.peakSpeed) << "%"
							  << std::setw(11) << Percent(a.distance, b.distance) << "%"
							  << std::noshowpos << "\n";
				}
			}
		}
	}

	void Sweep(RunConfig base)
	{
		const int movement[] = { 125, 250, 333, 500, 1000 };
		const int render[] = { 60, 125, 144, 240, 333 };

		std::cout << "== " << ModeName(base.mode) << " g_speed " << base.gSpeed
				  << ", peak speed change from vanilla at the same com_maxfps ==\n";
		std::cout << std::setw(12) << "com_maxfps";
		for (int r : render)
			std::cout << std::setw(10) << r;
		std::cout << std::setw(12) << "(vanilla)" << "\n";

		for (int m : movement)
		{
			RunConfig vanilla = base;
			vanilla.comMaxFps = m;
			vanilla.timestep = false;
			vanilla.srMaxFps = 0;
			const Best a = FindBest(vanilla);

			std::cout << std::setw(12) << m;
			for (int r : render)
			{
				RunConfig split = vanilla;
				split.timestep = true;
				split.srMaxFps = r;
				const Best b = FindBest(split);

				std::ostringstream cell;
				cell << std::fixed << std::setprecision(1) << Percent(a.peakSpeed, b.peakSpeed) << "%";
				std::cout << std::setw(10) << cell.str();
			}
			std::cout << std::setw(12) << std::fixed << std::setprecision(1) << a.peakSpeed << "\n";
		}
		std::cout << "\n";
	}
}

int main(int argc, char** argv)
{
	Options options;

	try
	{
		if (!ParseArgs(argc, argv, options))
		{
			Usage();
			return 1;
		}
		if (options.selfTest)
			return SelfTest(options.config);
		if (options.matrix)
		{
			Matrix(options.config);
			return 0;
		}
		if (options.sweep)
		{
			Sweep(options.config);
			return 0;
		}
		if (options.compare)
		{
			Compare(options.config);
			return 0;
		}
		const RunResult result = Run(options.config);
		Print(options.config.timestep ? "timestep" : "vanilla", result);

		if (!options.csv.empty())
			WriteCsv(options.csv, result);
	}
	catch (const std::exception& error)
	{
		std::cerr << "error: " << error.what() << "\n";
		return 1;
	}
	return 0;
}
