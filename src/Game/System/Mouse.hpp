#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// Tells the window procedure how to keep processing a WM_INPUT message.
	enum class RawInput
	{
		Ignored,  // Unknown device, keep the default handling.
		Keyboard, // Keyboard event, the engine still has to parse it.
		Handled,  // Mouse event, the game must still see the message.
		Consumed  // Mouse event fully owned by IW3SR.
	};

	class GMouse
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Frame();

		static RawInput Process(uintptr_t rawInput);
		static bool IsLooking();

	private:
		static inline dvar_s* RawInputDvar = nullptr;
		static inline dvar_s* LegacyRawInputDvar = nullptr;
		static inline dvar_s* MouseDvar = nullptr;

		static inline bool Enabled = false;
		static inline bool Looking = false;
		static inline bool CursorHidden = false;
		static inline bool Overridden = false;

		static inline bool EngineInput = true;
		static inline bool EngineInitialized = false;

		static bool IsGameplay();
		static void SetEngineInput(bool state);
		static void SetLooking(bool state);
		static void Confine();
		static void Reseed();
		static void SetCursorHidden(bool state);
	};
}
