#include "Sim/Hand.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Sim
{
	Keys Keys::Parse(const std::string& name)
	{
		Keys keys;
		for (char c : name)
		{
			switch (c)
			{
			case 'w':
				keys.forward = true;
				break;
			case 's':
				keys.back = true;
				break;
			case 'a':
				keys.left = true;
				break;
			case 'd':
				keys.right = true;
				break;
			case '-':
				break;
			default:
				throw std::runtime_error("unknown movement key: " + name);
			}
		}
		return keys;
	}

	Hand::Hand(const HandConfig& config) : Config(config)
	{
		Build();
	}

	void Hand::Calibrate(float sensitivity, float yawScale)
	{
		const double perCount = static_cast<double>(sensitivity) * static_cast<double>(yawScale);
		DegreesPerCount = perCount > 0.0 ? perCount : 1.0;
	}

	// Signed area under the sweep. Alternating flips the sign each period, so the view zig-zags
	// about its starting angle rather than winding away from it, which is what a hop chain does.
	double Hand::DegreesAt(int time) const
	{
		const int elapsed = time - Config.warmUpMs;
		if (elapsed <= 0)
			return 0.0;

		if (Config.alternateMs <= 0)
			return Config.yawRate * elapsed * 0.001;

		const int period = Config.alternateMs;
		const int index = elapsed / period;
		const int remainder = elapsed % period;

		// Whole periods cancel in pairs, so an even count contributes nothing.
		const double whole = index % 2 ? period : 0.0;
		const double partial = index % 2 ? -remainder : remainder;

		return Config.yawRate * (whole + partial) * 0.001;
	}

	int Hand::CountsAt(int time) const
	{
		return static_cast<int>(std::floor(DegreesAt(time) / DegreesPerCount));
	}

	int Hand::YawCountsBetween(int from, int to) const
	{
		return CountsAt(to) - CountsAt(from);
	}

	void Hand::Build()
	{
		const int start = Config.warmUpMs;

		if (Config.keys.forward)
			Schedule.push_back({ start, KB_FORWARD, true });
		if (Config.keys.back)
			Schedule.push_back({ start, KB_BACK, true });

		const bool strafing = Config.keys.left || Config.keys.right;

		if (Config.alternateMs > 0 && strafing)
		{
			// The strafe key follows the sweep, so the pair stays in the geometry that gains.
			for (int index = 0; start + index * Config.alternateMs < Config.durationMs; index++)
			{
				const int time = start + index * Config.alternateMs;
				const KeyButton held = index % 2 ? KB_MOVERIGHT : KB_MOVELEFT;
				const KeyButton released = index % 2 ? KB_MOVELEFT : KB_MOVERIGHT;

				if (index)
					Schedule.push_back({ std::max(start, time - Config.beatGapMs), released, false });
				Schedule.push_back({ time, held, true });
			}
		}
		else
		{
			if (Config.keys.left)
				Schedule.push_back({ start, KB_MOVELEFT, true });
			if (Config.keys.right)
				Schedule.push_back({ start, KB_MOVERIGHT, true });
		}

		if (Config.techniquePeriodMs > 0 && Config.keys.forward)
		{
			for (int index = 1; start + index * Config.techniquePeriodMs < Config.durationMs; index++)
				Schedule.push_back({ start + index * Config.techniquePeriodMs, KB_FORWARD, index % 2 == 0 });
		}

		switch (Config.jump)
		{
		case Jump::None:
			break;
		case Jump::Held:
			Schedule.push_back({ start, KB_JUMP, true });
			break;
		case Jump::Tapped:
			// A tap rather than a hold, so a mode that needs a button edge to rebound gets one.
			for (int time = start; time < Config.durationMs; time += Config.jumpPeriodMs)
			{
				Schedule.push_back({ time, KB_JUMP, true });
				Schedule.push_back({ time + Config.jumpPeriodMs / 2, KB_JUMP, false });
			}
			break;
		}

		std::stable_sort(Schedule.begin(), Schedule.end(),
			[](const KeyEvent& a, const KeyEvent& b) { return a.time < b.time; });
	}
}
