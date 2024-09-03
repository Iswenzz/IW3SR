#include "System.hpp"

#include "Core/System/Window.hpp"

namespace IW3SR
{
	HWND GSystem::CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X,
		int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		HWND hwnd = CreateWindowExA_h(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
			hMenu, hInstance, lpParam);

		const std::string_view windowName = lpWindowName;
		if (windowName == "Call of Duty 4" || windowName == "Call of Duty 4 X")
			OSWindow::Handle = hwnd;
		return hwnd;
	}

	LRESULT GSystem::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			Keyboard::Process(msg, wParam);
			break;
		}
		if (UI::Active)
		{
			if (UI::Open && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
				return true;

			ImGuiIO& io = ImGui::GetIO();
			io.MouseDrawCursor = UI::Open;
		}
		s_wmv->mouseInitialized = !UI::Open;

		return UI::Open ? DefWindowProc(hWnd, msg, wParam, lParam) : MainWndProc_h(hWnd, msg, wParam, lParam);
	}

	void GSystem::ExecuteSingleCommand(int localClientNum, int controllerIndex, char* cmd)
	{
		std::string command = cmd;
		Cmd_ExecuteSingleCommand_h(localClientNum, controllerIndex, cmd);

		if (command == "openscriptmenu cj load")
		{
			EventClientLoadPosition event;
			Application::Dispatch(event);
		}
		EventClientCommand event(command);
		Application::Dispatch(event);
	}

	void GSystem::ScriptMenuResponse(int localClientNum, itemDef_s* item, const char** args)
	{
		std::string arguments = *args;
		const char* data = arguments.data();
		std::string response = Com_ParseExt(&data, false);

		Script_ScriptMenuResponse_h(localClientNum, item, args);

		EventScriptMenuResponse event(item->parent->window.name, response);
		Application::Dispatch(event);
	}
}
