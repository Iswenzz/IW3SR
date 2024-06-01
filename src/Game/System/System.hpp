#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Game system.
	/// </summary>
	class GSystem
	{
	public:
		/// <summary>
		/// Creates the main window.
		/// </summary>
		/// <param name="dwExStyle">The extended window style.</param>
		/// <param name="lpClassName">The window class name.</param>
		/// <param name="lpWindowName">The window title.</param>
		/// <param name="dwStyle">The window style.</param>
		/// <param name="X">The X-coordinate of the window.</param>
		/// <param name="Y">The Y-coordinate of the window.</param>
		/// <param name="nWidth">The width of the window.</param>
		/// <param name="nHeight">The height of the window.</param>
		/// <param name="hWndParent">The parent window handle.</param>
		/// <param name="hMenu">The menu handle.</param>
		/// <param name="hInstance">The instance handle.</param>
		/// <param name="lpParam">A pointer to user-defined data.</param>
		/// <returns>The handle to the created main window.</returns>
		static HWND STDCALL CreateMainWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
			int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

		/// <summary>
		/// Window procedure for the main window.
		/// </summary>
		/// <param name="hWnd">The handle to the window.</param>
		/// <param name="msg">The message code.</param>
		/// <param name="wParam">Additional message-specific information.</param>
		/// <param name="lParam">Additional message-specific information.</param>
		/// <returns>The result of processing the message.</returns>
		static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		/// <summary>
		/// Execute single command.
		/// </summary>
		/// <param name="localClientNum">The local client number.</param>
		/// <param name="controllerIndex">The controller index.</param>
		/// <param name="command">The command.</param>
		static void ExecuteSingleCommand(int localClientNum, int controllerIndex, char *command);

		/// <summary>
		/// Script menu response.
		/// </summary>
		/// <param name="localClientNum">The local client number.</param>
		/// <param name="item">The ui item.</param>
		/// <param name="args">The args.</param>
		static void ScriptMenuResponse(int localClientNum, itemDef_s *item, const char **args);
	};
}
