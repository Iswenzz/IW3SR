#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max

#include <cef_app.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	void* sandboxInfo = nullptr;
	//CefScopedSandboxInfo scopedSandbox;
	//sandboxInfo = scopedSandbox.sandbox_info();

	CefMainArgs args(hInstance);
	return CefExecuteProcess(args, nullptr, sandboxInfo);
}
