#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// A key bind command held back until the movement step it actually happened in. The engine
	// stamps every one of them with the millisecond the key really went down or up.
	struct KeyEvent
	{
		std::string Command;
		int LocalClientNum = 0;
		int ControllerIndex = 0;
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

		static void FASTCALL CreateNewCommands(int localClientNum);
		static bool Defer(int localClientNum, int controllerIndex, const std::string& command);
		static void StartTest();
		static void Status();

		static int MovementFps();
		static int RenderFps();
		static int DisplayFps();

	private:
		static bool Active();
		static void Split(int localClientNum);
		static void Replay(int time, bool placed);
		static void Flush();
		static void Record(const usercmd_s& cmd);
		static void Audit();
		static void TestFrame();
		static void Send(const char* command, int key, int time);

		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* MaxFps = nullptr;
		static inline dvar_s* ComMaxFps = nullptr;
		static inline dvar_s* Log = nullptr;

		static inline dvar_s** Limiter = nullptr;
		static inline dvar_s Limit = {};

		static inline std::deque<KeyEvent> Events;
		static inline bool Replaying = false;
		static inline std::ofstream Journal;
		static inline int Logged = 0;
		static inline int Previous = 0;
		static inline int Time = 0;
		static inline int Starved = 0;
		static inline int Offset = 0;
		static inline int Placed = 0;
		static inline int Late = 0;
		static inline bool Warned = false;
		static inline int Emitted = 0;
		static inline int Wobble = 0;
		static inline int Steady = 0;
		static inline int Test = 0;
		static inline int Beat = 0;
		static inline int Peak = 0;
	};
}
