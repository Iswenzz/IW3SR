#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// The client's frame limiter, kept going as a clock of movement steps.
	struct Cadence
	{
		int Time = 0;
		bool Started = false;
	};

	// Steps run from Clock, each as wide as its entry in the widths array.
	struct Steps
	{
		int Clock = 0;
		int Count = 0;
		bool Starved = false;
	};

	// The server clock of a client at com_maxfps: what CL_SetCGameTime and CL_AdjustTimeDelta keep,
	// run once per movement step instead of once per rendered frame.
	struct ServerClock
	{
		int Delta = 0;
		int OldServerTime = 0;
		int Snap = 0;
		int OldSnap = 0;
		bool Extrapolated = false;
		bool Started = false;
	};

	// A snapshot and the step clock time it is reckoned to have reached the client at.
	struct Arrival
	{
		int Snap = 0;
		int Time = 0;
	};

	// Splits every rendered frame into fixed size movement commands so the physics rate follows
	// com_maxfps instead of the frame rate the machine happens to reach. sr_maxfps caps the
	// renderer on its own, which leaves com_maxfps free to mean nothing but the movement rate.
	class API Timestep
	{
	public:
		static void Initialize();
		static void Frame();
		static void Reset();
		static bool Command(const std::string& command);

		static void FASTCALL CreateNewCommands(int localClientNum);
		static void CalcViewValues(int localClientNum);
		static void Sample(const usercmd_s& cmd);
		static void Step(const pmove_t* pm, const pml_t* pml);
		static void Bounce(const pmove_t* pm, const pml_t* pml, const trace_t& trace, float before);

		static int MovementFps();
		static int RenderFps();
		static int DisplayFps();

	private:
		static bool Active();
		static void Split(int localClientNum);
		static Steps PlanSteps(int* widths, int step, int target, int frametime);
		static void SeedClock(int target);
		static void Receive(int target, int frametime);
		static int Tick(int time);
		static void AdjustDelta(int time);
		static float JumpApex(float velocity, float gravity, int msec);
		static void CheckPackets();
		static void TrackJump(const playerState_s& ps);
		static float IdealApex(const playerState_s& ps);
		static void Status();
		static void Record(const usercmd_s& cmd);
		static void StartTest();
		static void TestFrame();
		static void Send(const char* command, int key, int time);

		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* MaxFps = nullptr;
		static inline dvar_s* ComMaxFps = nullptr;
		static inline dvar_s* Smooth = nullptr;
		static inline dvar_s* Log = nullptr;
		static inline dvar_s* Apex = nullptr;

		// The client limiter the step widths come from.
		static inline Cadence Vanilla = {};

		// The server clock the command times come from, and the snapshots it has yet to see.
		static inline ServerClock Server = {};
		static inline std::vector<Arrival> Arrivals;
		static inline int Seen = 0;
		static inline double Offset = 0;
		static inline bool HasOffset = false;

		static inline dvar_s** Limiter = nullptr;
		static inline dvar_s Limit = {};

		// The command as the engine built it, before any module had a say in it.
		static inline usercmd_s Raw = {};

		static inline std::ofstream Trace;
		static inline int Stepped = 0;

		static inline std::ofstream Journal;
		static inline int Logged = 0;
		static inline int Previous = 0;

		static inline int Starved = 0;
		static inline int Sent = 0;
		static inline bool Dropping = false;

		// Steps emitted and the stretch of the step clock they covered, for the rate actually reached.
		static inline int Emitted = 0;
		static inline int First = 0;
		static inline int Last = 0;

		// com_frameTime as the last step was handed it, which the next step's frame_msec runs from.
		static inline int Stamp = 0;

		// The jump in flight, measured from takeoff, and the last one to land.
		static inline bool Grounded = true;
		static inline bool Jumping = false;
		static inline float JumpBase = 0;
		static inline float JumpTop = 0;
		static inline float LastApex = 0;
		static inline float LastIdeal = 0;

		static inline int Test = 0;
		static inline int Beat = 0;
		static inline int Peak = 0;
	};
}
