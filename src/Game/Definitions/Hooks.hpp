#pragma once
#include "Structs.hpp"

#include "Core/Memory/Assembler.hpp"
#include "Core/Memory/Hook.hpp"

// clang-format off
namespace IW3SR
{
	extern Hook<HWND STDCALL(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
		DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
		HINSTANCE hInstance, LPVOID lpParam)>
		CreateWindowExA_h;

	extern Hook<IDirect3D9* STDCALL(UINT sdk)>
		Direct3DCreate9_h;

	extern Hook<int(char *dest, size_t size, const char *fmt, va_list va)>
		Vsnprintf_h;

	extern Hook<LRESULT CALLBACK(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)>
		MainWndProc_h;

	extern Hook<void(int localClientNum, int controllerIndex, char* command)>
		Cmd_ExecuteSingleCommand_h;

	extern Hook<void(ConChannel channel, const char* msg, int type)>
		Com_PrintMessage_h;

	extern Hook<void(int localClientNum)>
		CG_DrawCrosshair_h;

	extern Hook<void(int localClientNum)>
		CG_PredictPlayerState_Internal_h;

	extern Hook<void()>
		CG_Respawn_h;

	extern Hook<void()>
		CL_Connect_h;

	extern Hook<void(int localClientNum)>
		CL_Disconnect_h;

	extern Hook<void(usercmd_s* cmd)>
		CL_FinishMove_h;

	extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_WalkMove_h;

	extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_AirMove_h;

	extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace_h;

	extern Hook<void()>
		R_Init_h;

	extern Hook<void(void* cmds)>
		R_RenderAllLeftovers_h;

	extern Hook<void(int window)>
		R_Shutdown_h;

	extern Hook<void(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)>
		RB_EndSceneRendering_h;

	extern Hook<void(int localClientNum, itemDef_s *item, const char **args)>
		Script_ScriptMenuResponse_h;
}
// clang-format on
namespace IW3SR
{
	ASM_FUNCTION(R_RenderAllLeftovers_h);
}
