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

	extern Hook<void(int localClientNum, int controllerIndex, char* command)>
		Cmd_ExecuteSingleCommand_h;

	extern Hook<void()>
		Com_PlayIntroMovies_h;

	extern Hook<void(ConChannel channel, const char* msg, int type)>
		Com_PrintMessage_h;

	extern Hook<void(int localClientNum)>
		CG_DrawCrosshair_h;

	extern Hook<void(int localClientNum)>
		CG_PredictPlayerState_Internal_h;

	extern Hook<void(int localClientNum)>
		CG_Respawn_h;

	extern Hook<void(int localClientNum)>
		CL_InitCGame_h;

	extern Hook<void(int localClientNum)>
		CL_Shutdown_h;

	extern Hook<void()>
		CL_Connect_h;

	extern Hook<void(int localClientNum)>
		CL_Disconnect_h;

	extern Hook<void(usercmd_s* cmd)>
		CL_FinishMove_h;

	extern Hook<HRESULT STDCALL(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)>
		IDirect3DDevice9_Reset_h;

	extern Hook<void STDCALL(IDirect3DDevice9* device)>
		IDirect3DDevice9_EndScene_h;

	extern Hook<HMODULE STDCALL(LPCSTR lpLibFileName)>
		LoadLibraryA_h;

	extern Hook<HMODULE STDCALL(LPCWSTR lpLibFileName)>
		LoadLibraryW_h;

	extern Hook<LRESULT CALLBACK(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)>
		MainWndProc_h;

	extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_WalkMove_h;

	extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_AirMove_h;

	extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace_h;

	extern Hook<void(const char** text, int maxChars, Font_s* font, float x, float y, float xScale, float yScale, float rotation,
		int style, const vec4& color)>
		R_AddCmdDrawText_h;

	extern Hook<void(const char* text, int maxChars, Font_s* font, float x, float y, float xScale, float yScale, float rotation,
		const vec4& color, int style, const vec4& glowColor, Material* fxMaterial, Material* fxMaterialGlow,
		int fxBirthTime, int fxLetterTime, int fxDecayStartTime, int fxDecayDuration)>
		R_AddCmdDrawTextWithEffects_h;

	extern Hook<void()>
		R_Init_h;

	extern Hook<void(int window)>
		R_Shutdown_h;

	extern Hook<void(void* cmds)>
		RB_ExecuteRenderCommandsLoop_h;

	extern Hook<void(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)>
		RB_EndSceneRendering_h;

	extern Hook<void(int localClientNum, itemDef_s *item, const char **args)>
		Script_ScriptMenuResponse_h;

	extern Hook<int(char *dest, size_t size, const char *fmt, va_list va)>
		Vsnprintf_h;

	extern Hook<void()>
		XAssetsInitStdCount_h;
}
// clang-format on
namespace IW3SR
{
	ASM_FUNCTION(CL_Shutdown_h);
	ASM_FUNCTION(CG_Respawn_h);
	ASM_FUNCTION(R_AddCmdDrawText_h);
	ASM_FUNCTION(RB_ExecuteRenderCommandsLoop_h);
}
