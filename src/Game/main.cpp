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
