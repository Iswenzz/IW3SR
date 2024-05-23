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
		auto& UI = UI::Get();

		switch (msg)
		{
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			Keyboard::Process(msg, wParam);
			break;
		}
		if (!UI.Active)
			return MainWndProc_h(hWnd, msg, wParam, lParam);

		ImGuiIO& io = ImGui::GetIO();
		io.MouseDrawCursor = UI.Open;
		s_wmv->mouseInitialized = !UI.Open;

		if (UI.Open)
		{
			ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
			return true;
		}
		return MainWndProc_h(hWnd, msg, wParam, lParam);
	}

	int GSystem::Vsnprintf(char* dest, size_t size, const char* fmt, va_list va)
	{
		if (!fmt)
			return _vsnprintf(dest, size, fmt, va);

		const std::string_view format = fmt;

		// Singleplayer maps
		if (format.contains("maps/mp/"))
		{
			va_list args;
			va_copy(args, va);

			if (format == "maps/mp/%s.d3dbsp")
			{
				std::string_view map = va_arg(args, char*);
				if (!map.starts_with("mp_"))
					fmt = "maps/%s.d3dbsp";
			}
			va_end(args);
		}
		return _vsnprintf(dest, size, fmt, va);
	}

	void GSystem::ExecuteSingleCommand(int localClientNum, int controllerIndex, char* cmd)
	{
		std::string command = cmd;
		Cmd_ExecuteSingleCommand_h(localClientNum, controllerIndex, cmd);

		if (command == "openscriptmenu cj load")
		{
			EventClientLoadPosition event;
			Application::Get().Dispatch(event);
		}
		EventClientCommand event(command);
		Application::Get().Dispatch(event);
	}

	void GSystem::ScriptMenuResponse(int localClientNum, itemDef_s* item, const char** args)
	{
		std::string arguments = *args;
		const char* data = arguments.data();
		std::string response = Com_ParseExt(&data, false);

		Script_ScriptMenuResponse_h(localClientNum, item, args);

		EventScriptMenuResponse event(item->parent->window.name, response);
		Application::Get().Dispatch(event);
	}
}
