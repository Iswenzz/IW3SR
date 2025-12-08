#include "Game/Base.hpp"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		Application::Start();
		break;

	case DLL_PROCESS_DETACH:
		Application::Shutdown();
		break;
	}
	return TRUE;
}

static HMODULE library =
	LoadLibrary((std::filesystem::path(std::getenv("WINDIR")) / "System32" / "ddraw.dll").string().c_str());

#undef PROXY_LIBRARY
#define PROXY_LIBRARY library

// clang-format off
PROXY(WINAPI, HRESULT, DirectDrawCreateEx,
	(GUID *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkOuter),
	(lpGUID, lplpDD, iid, pUnkOuter))

PROXY(WINAPI, HRESULT, DirectDrawEnumerateExA,
	(LPVOID lpCallback, LPVOID lpContext, DWORD dwFlags),
	(lpCallback, lpContext, dwFlags))
// clang-format on
