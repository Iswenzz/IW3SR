#include "Game/Base.hpp"

static HMODULE library =
	LoadLibrary((std::filesystem::path(std::getenv("WINDIR")) / "System32" / "ddraw.dll").string().c_str());

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		Application::Start();
		std::cout << "{:x}" << library;
		break;
	case DLL_PROCESS_DETACH:
		Application::Shutdown();
		break;
	}
	return TRUE;
}

#undef PROXY_LIBRARY
#define PROXY_LIBRARY library

// clang-format off
PROXY(WINAPI, HRESULT, DirectDrawCreateEx,
	(void *lpGUID, void **lplpDD, void *iid, void *pUnkOuter),
	(lpGUID, lplpDD, iid, pUnkOuter))

PROXY(WINAPI, HRESULT, DirectDrawEnumerateExA,
	(void *lpCallback, void *lpContext, DWORD dwFlags),
	(lpCallback, lpContext, dwFlags))
// clang-format on
