#include "System.hpp"
#include "Patch.hpp"

namespace IW3SR
{
	void GSystem::Initialize()
	{
		Com_PlayIntroMovies_h();

		Dvar::Initialize();

		for (int i = 0; i <= dvarCount - 1; i++)
			Console::AddCommand(dvars[i]->name);

		auto& players = Player::GetAll();
		for (int i = 0; i < players.size(); i++)
			players[i] = CreateRef<Player>(i);
	}

	void GSystem::Shutdown() { }

	HWND GSystem::CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X,
		int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		HWND hwnd = CreateWindowExA_h(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
			hMenu, hInstance, lpParam);

		if (!lpWindowName)
			return hwnd;

		const std::string_view windowName = lpWindowName;
		if (windowName == "Call of Duty 4" || windowName == "Call of Duty 4 X")
		{
			Window::Swap(hwnd);
			SetWindowText(hwnd, "IW3SR");
		}
		return hwnd;
	}

	LRESULT GSystem::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INPUT:
			Mouse::Process(msg, lParam);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			Keyboard::Process(msg, wParam);
			break;
		}
		if (UI::KeyOpen.IsPressed())
			return true;
		if (UI::Open && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;
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

	HMODULE GSystem::LoadDLL(LPCSTR lpLibFileName)
	{
		const HMODULE mod = LoadLibraryA_h(lpLibFileName);
		const std::string name = std::filesystem::path(lpLibFileName).filename().string();

		if (name == "gdi32.dll")
			Patch::Base();
		return mod;
	}

	HMODULE STDCALL GSystem::LoadDLLW(LPCWSTR lpLibFileName)
	{
		const HMODULE mod = LoadLibraryW_h(lpLibFileName);
		const std::string name = std::filesystem::path(lpLibFileName).filename().string();

		if (name.starts_with("cod4x"))
			Patch::CoD4X(mod);
		return mod;
	}
}
