#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GCdKey
	{
	public:
		static void Initialize();
		static void Protect();
		static void Frame();

	private:
		static inline dvar_s* EnabledDvar = nullptr;
		static inline dvar_s* Fragments[5] = {};
		static inline bool Warned = false;
		static inline bool Unwired = false;

		static bool IsEnabled();
		static void Resolve();
		static bool Blank(const dvar_s* dvar);
		static bool Wipe(const char* string);
		static bool Writable(const void* address, size_t size);
	};
}
