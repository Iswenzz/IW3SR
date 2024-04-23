#include "Game/Base.hpp"

void Init()
{
	Application::Get().Start();
}

void Shutdown()
{
	Application::Get().Shutdown();
}

ENTRY BOOL STDCALL RIB_Main(HANDLE handle, INT upDown)
{
	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		Shutdown();
		break;
	default:
		break;
	}
	return TRUE;
}
