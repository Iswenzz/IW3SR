#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	struct PatchZone
	{
		std::string Name;
		std::string File;
		std::filesystem::path Path;
	};

	class GZones
	{
	public:
		static void Inject(std::vector<XZoneInfo>& zones);
		static void Shutdown();

		static std::filesystem::path Root();
		static int FileSize(const char* name, int size);

	private:
		static inline dvar_s* Enabled = nullptr;
		static inline Hook<HANDLE STDCALL(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE)> Redirect;

		static inline std::vector<PatchZone> Layer;
		static inline bool Discovered = false;
		static inline bool Injected = false;

		static void Discover();
		static void PatchFileSize();
		static void Collect();
		static void MountIwd();
		static bool Resolvable(const PatchZone& zone);
		static const PatchZone* Match(std::string_view path);

		static HANDLE STDCALL OpenFile(LPCSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
			DWORD creation, DWORD flags, HANDLE templ);
	};
}
