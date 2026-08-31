#include "System.hpp"
#include "AssetDump.hpp"
#include "Assets.hpp"
#include "Capture.hpp"
#include "CdKey.hpp"
#include "Channel.hpp"
#include "Demo.hpp"
#include "Discord.hpp"
#include "Download.hpp"
#include "Dvar.hpp"
#include "Master.hpp"
#include "Mouse.hpp"
#include "NetSim.hpp"
#include "Patch.hpp"
#include "Protocol.hpp"
#include "QoS.hpp"
#include "Rank.hpp"
#include "Rcon.hpp"
#include "ServerFilter.hpp"
#include "Shell.hpp"
#include "Sound.hpp"
#include "Timestep.hpp"
#include "Zones.hpp"

namespace IW3SR
{
	void GSystem::Initialize()
	{
		if (Initialized)
			return;
		Initialized = true;

		Dvar::Initialize();
		GShell::Initialize();
		GProtocol::Initialize();
		NetSim::Initialize();
		GRcon::Initialize();
		GMaster::Initialize();
		GQoS::Initialize();
		ServerFilter::Initialize();
		GDownload::Initialize();
		GChannel::Initialize();
		GCdKey::Initialize();
		Timestep::Initialize();
		Assets::Initialize();
		GRank::Initialize();
		GAssetDump::Initialize();
		Capture::Initialize();
		Demo::Initialize();
		GMouse::Initialize();
		GDiscord::Initialize();
	}

	void GSystem::Shutdown(int localClientNum)
	{
		if (IsShutdown)
			return;
		IsShutdown = true;

		NetSim::Shutdown();
		GRcon::Shutdown();
		GMaster::Shutdown();
		GQoS::Shutdown();
		GDiscord::Shutdown();
		GDownload::Shutdown();
		GZones::Shutdown();

		Capture::Shutdown();
		GMouse::Shutdown();
		Browser::Shutdown();
		Application::Shutdown();

		GChannel::Shutdown();
		GProtocol::Shutdown();
		Dvar::Unregister();
	}

	void GSystem::MainLoop(int tickRate)
	{
		PbServerProcessEvents_h(tickRate);

		GMouse::Frame();
		Timestep::Frame();
		Capture::Tick();
		Demo::Tick();
		GCdKey::Frame();
		GMaster::Frame();
		GRank::Frame();
		GRcon::Frame();
		GDiscord::Frame();
		GDownload::Frame();
		GChannel::Frame();

		GProtocol::Frame();
		GSystem::Tasks.Submit();

		if (ExitRequested)
			Cmd_ExecuteSingleCommand(0, 0, "quit\n");
	}

	static bool Borderless(LPCSTR name, DWORD& style, int& x, int& y, int& width, int& height)
	{
		if (!name || std::string_view(name) != "CoD4" || (style & WS_CAPTION) == 0)
			return false;

		const POINT corner = { x, y };
		const HMONITOR monitor = MonitorFromPoint(corner, MONITOR_DEFAULTTOPRIMARY);

		MONITORINFO info = { sizeof(info) };
		if (!monitor || !GetMonitorInfoA(monitor, &info))
			return false;

		const int monitorWidth = info.rcMonitor.right - info.rcMonitor.left;
		const int monitorHeight = info.rcMonitor.bottom - info.rcMonitor.top;

		if (width < monitorWidth || height < monitorHeight)
			return false;

		style = WS_POPUP;
		x = info.rcMonitor.left;
		y = info.rcMonitor.top;
		width = monitorWidth;
		height = monitorHeight;
		return true;
	}

	HWND GSystem::CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X,
		int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		Borderless(lpClassName, dwStyle, X, Y, nWidth, nHeight);

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
		{
			const RawInput result = GMouse::Process(lParam);
			if (result == RawInput::Keyboard)
				Keyboard::Process(msg, lParam);
			else if (result == RawInput::Consumed)
				return DefWindowProc(hWnd, msg, wParam, lParam);
			break;
		}

		case WM_MOUSEMOVE:
			Mouse::Process(msg, lParam);
			break;

		case WM_CHAR:
			Keyboard::Process(msg, wParam);
			break;

		case WM_SIZE:
			Window::Size = vec2(LOWORD(lParam), HIWORD(lParam));
			Window::IsMinimized = wParam == SIZE_MINIMIZED;
			if (Renderer::Active && !Window::IsMinimized)
				Renderer::Resize(Window::Size);
			break;

		case WM_CLOSE:
			UI::Open = false;
			break;
		}
		if (UI::KeyOpen.IsPressed())
			return true;
		if (UI::Open && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;
		if (UI::Active)
			s_wmv->mouseInitialized = !UI::Open;
		return UI::Open ? DefWindowProc(hWnd, msg, wParam, lParam) : MainWndProc_h(hWnd, msg, wParam, lParam);
	}

	void GSystem::ExecuteSingleCommand(int localClientNum, int controllerIndex, char* cmd)
	{
		std::string command = cmd;

		if (Capture::Command(command))
			return;
		if (GShell::Command(command))
			return;
		if (Demo::Command(command))
			return;
		if (GSound::Command(command))
			return;
		if (GRcon::Command(command))
			return;
		if (GMaster::Command(command))
			return;
		if (ServerFilter::Command(command))
			return;
		if (GAssetDump::Command(command))
			return;
		if (GDownload::Command(command))
			return;

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

	HMODULE GSystem::LoadDLLW(LPCWSTR lpLibFileName)
	{
		const HMODULE mod = LoadLibraryW_h(lpLibFileName);
		const std::string name = std::filesystem::path(lpLibFileName).filename().string();

		if (name == "launcher.dll" && Patch::DisableCoD4X)
			FreeLibrary(mod);
		if (name.starts_with("cod4x"))
			Patch::CoD4X(mod);
		return mod;
	}

	HMODULE GSystem::LoadDLLExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
	{
		const HMODULE mod = LoadLibraryExW_h(lpLibFileName, hFile, dwFlags);
		const std::string name = std::filesystem::path(lpLibFileName).filename().string();

		if (name.starts_with("cod4x"))
			Patch::CoD4X(mod);
		return mod;
	}

	int GSystem::Vsnprintf(char* dest, size_t size, const char* fmt, va_list va)
	{
		if (!fmt || !dest)
			return -1;

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
}
