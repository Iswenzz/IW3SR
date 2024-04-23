#include "System.hpp"

#include "Core/System/System.hpp"

namespace IW3SR
{
	HWND GSystem::CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X,
		int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		HWND hwnd = CreateWindowExA_h(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
			hMenu, hInstance, lpParam);

		std::string windowName = std::string{ lpWindowName };
		if (windowName != "Call of Duty 4" && windowName != "Call of Duty 4 X")
			return hwnd;

		return reinterpret_cast<HWND>(System::MainWindow = hwnd);
	}

	LRESULT GSystem::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto& UI = UI::Get();
		Keyboard::Process(msg, wParam);

		if (!UI.Active)
			return MainWndProc_h(hWnd, msg, wParam, lParam);

		ImGuiIO& io = ImGui::GetIO();
		if (UI.Open)
		{
			if (Keyboard::IsPressed(Key_Escape))
				UI.Open = false;
			ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
			io.MouseDrawCursor = true;
			return true;
		}
		io.MouseDrawCursor = false;
		return MainWndProc_h(hWnd, msg, wParam, lParam);
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
		EventExecuteCommand event(command);
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
