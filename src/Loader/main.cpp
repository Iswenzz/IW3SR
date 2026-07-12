#include <windows.h>
#include <filesystem>
#include <string>

static std::filesystem::path ExeDirectory()
{
	char exe[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, exe, MAX_PATH);
	return std::filesystem::path(exe).parent_path();
}

static bool IsGameProcess()
{
	char exe[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, exe, MAX_PATH);
	std::string name = std::filesystem::path(exe).filename().string();
	for (char& c : name)
		c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	return name == "iw3mp.exe";
}

static HMODULE RealLibrary()
{
	static HMODULE library = []
	{
		char system32[MAX_PATH] = { 0 };
		GetSystemDirectory(system32, MAX_PATH);
		return LoadLibrary((std::filesystem::path(system32) / "ddraw.dll").string().c_str());
	}();
	return library;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	static HMODULE module = nullptr;

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);
		if (!IsGameProcess())
			return TRUE;

		const auto bin = ExeDirectory() / "iw3sr" / "Bin";
		SetDllDirectory(bin.string().c_str());
		module = LoadLibrary((bin / "IW3SR.dll").string().c_str());
		break;
	}
	case DLL_PROCESS_DETACH:
		if (!lpReserved && module)
			FreeLibrary(module);
		break;
	}
	return TRUE;
}

#define PROXY(CallingConv, ReturnType, FuncName, Params, Args)                  \
	__declspec(dllexport) ReturnType CallingConv _##FuncName Params             \
	{                                                                           \
		static auto address = GetProcAddress(RealLibrary(), #FuncName);         \
		if (!address)                                                           \
			return E_FAIL;                                                      \
		return reinterpret_cast<ReturnType(CallingConv*) Params>(address) Args; \
	}

// clang-format off
PROXY(WINAPI, HRESULT, DirectDrawCreateEx,
	(GUID *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkOuter),
	(lpGUID, lplpDD, iid, pUnkOuter))

PROXY(WINAPI, HRESULT, DirectDrawEnumerateExA,
	(LPVOID lpCallback, LPVOID lpContext, DWORD dwFlags),
	(lpCallback, lpContext, dwFlags))
// clang-format on
