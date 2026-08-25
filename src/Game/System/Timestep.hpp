#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
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

		static int MovementFps();
		static int RenderFps();
		static int DisplayFps();

	private:
		static bool Active();
		static void Split(int slot, const usercmd_s& previous);

		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* MaxFps = nullptr;
		static inline dvar_s* ComMaxFps = nullptr;

		static inline dvar_s** Limiter = nullptr;
		static inline dvar_s Limit = {};

		static inline int Time = 0;
		static inline int Angles[3] = {};
	};
}
