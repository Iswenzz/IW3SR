#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GSystem
	{
	public:
		static inline bool ExitRequested = false;

		static void Initialize();
		static void Shutdown(int localClientNum);

		static HWND STDCALL CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
			int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
		static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static void ExecuteSingleCommand(int localClientNum, int controllerIndex, char *command);
		static void ScriptMenuResponse(int localClientNum, itemDef_s *item, const char **args);
		static HMODULE STDCALL LoadDLL(LPCSTR lpLibFileName);
		static HMODULE STDCALL LoadDLLW(LPCWSTR lpLibFileName);
		static int Vsnprintf(char *dest, size_t size, const char *fmt, va_list va);
	};
}
