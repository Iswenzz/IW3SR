#pragma once
// The player, as a function of time. Both clients are driven from one of these so a comparison
// between them is a comparison of the clients and not of two different runs of the keyboard.
#include "Engine/Client.hpp"

#include <string>
#include <vector>

namespace Sim
{
	struct KeyEvent
	{
		int time;
		KeyButton key;
		bool down;
	};

	// Which movement keys are held. Named the way a player says them: wa, wd, a, d, s, sa, sd.
	struct Keys
	{
		bool forward = false;
		bool back = false;
		bool left = false;
		bool right = false;

		static Keys Parse(const std::string& name);
	};

	enum class Jump
	{
		None,
		Held,
		Tapped,
	};

	struct HandConfig
	{
		Keys keys;
		Jump jump = Jump::Held;
		int jumpPeriodMs = 400;

		// Swaps the strafe key and the direction of the sweep on this period, the zig-zag a hop
		// chain is. Zero holds one direction for the whole run.
		int alternateMs = 0;

		// Milliseconds between releasing one strafe key and pressing the other. A real hand
		// leaves a gap here; it is the window "Remove Strafe Downtime" exists to fill.
		int beatGapMs = 0;

		// Toggles the forward key on this period, so a run mixes full beats and half beats the
		// way a real one does. The best com_maxfps differs between the two, so a fixed rate
		// cannot be right for both.
		int techniquePeriodMs = 0;

		// Degrees a second, positive turning left. A strafe only gains while the view leads the
		// velocity, so this is the parameter a scenario is really swept over.
		float yawRate = 0.0f;

		int warmUpMs = 100;
		int durationMs = 3000;
	};

	// A mouse reports whole counts, so the yaw a client sees is quantised however finely it asks.
	// Cumulative counts are a pure function of time: both clients draw from the same stream no
	// matter when or how often they drain it, and draining two disjoint spans adds up to draining
	// the whole. That is what makes the comparison a comparison of clients.
	class Hand
	{
	public:
		explicit Hand(const HandConfig& config);

		int CountsAt(int time) const;
		int YawCountsBetween(int from, int to) const;

		const std::vector<KeyEvent>& Events() const
		{
			return Schedule;
		}

		// A count moves the view by sensitivity * yawScale degrees, so the sweep rate means
		// nothing until it is expressed in the client's units.
		void Calibrate(float sensitivity, float yawScale);

	private:
		void Build();
		double DegreesAt(int time) const;

		HandConfig Config;
		double DegreesPerCount = 1.0;
		std::vector<KeyEvent> Schedule;
	};
}
