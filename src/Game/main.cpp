#include "Game/Base.hpp"

ENTRY BOOL STDCALL RIB_Main(HANDLE handle, INT upDown)
{
	return TRUE;
}

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
