#include <windows.h>
#include <filesystem>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	static HMODULE module = nullptr;

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		SetDllDirectory("iw3sr/Bin");
		module = LoadLibrary("IW3SR.dll");
		break;

	case DLL_PROCESS_DETACH:
		FreeLibrary(module);
		break;
	}
	return TRUE;
}

static HMODULE library =
	LoadLibrary((std::filesystem::path(std::getenv("WINDIR")) / "System32" / "ddraw.dll").string().c_str());

#define PROXY_LIBRARY library
#define PROXY(CallingConv, ReturnType, FuncName, Params, Args)                   \
	__declspec(dllexport) ReturnType CallingConv _##FuncName Params              \
	{                                                                            \
		static auto address = GetProcAddress(PROXY_LIBRARY, #FuncName);          \
		return reinterpret_cast<ReturnType(CallingConv *) Params>(address) Args; \
	}

// clang-format off
PROXY(WINAPI, HRESULT, DirectDrawCreateEx,
	(GUID *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkOuter),
	(lpGUID, lplpDD, iid, pUnkOuter))

PROXY(WINAPI, HRESULT, DirectDrawEnumerateExA,
	(LPVOID lpCallback, LPVOID lpContext, DWORD dwFlags),
	(lpCallback, lpContext, dwFlags))
// clang-format on
