#pragma once
#include "Structs.hpp"

#include "Engine/Core/Memory/Assembler.hpp"
#include "Engine/Core/Memory/Function.hpp"
#include "Engine/Core/Memory/Hook.hpp"

// clang-format off
namespace IW3SR
{
	extern Hook<HWND STDCALL(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
		DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
		HINSTANCE hInstance, LPVOID lpParam)>
		CreateWindowExA_h;

	extern Hook<void(int localClientNum, int controllerIndex, char* command)>
		Cmd_ExecuteSingleCommand_h;

	extern Hook<void(ConChannel channel, const char* msg, int type)>
		Com_PrintMessage_h;

	extern Hook<void(int localClientNum)>
		CG_CalcViewValues_h;

	extern Hook<void()>
		CG_CalculateFPS_h;

	extern Hook<void(int localClientNum)>
		CG_DrawCrosshair_h;

	extern Hook<void(int localClientNum)>
		CG_PredictPlayerState_Internal_h;

	extern Hook<void()>
		CG_RegisterItems_h;

	extern Hook<void(const char** weapons, int weaponCount)>
		CG_RegisterWeapons_h;

	extern Hook<void(int localClientNum)>
		CG_Respawn_h;

	extern Hook<void(int localClientNum)>
		CL_InitCGame_h;

	extern Hook<void(XZoneInfo* zoneInfo, unsigned int zoneCount, int sync)>
		DB_LoadXAssets_h;

	extern Hook<void(int localClientNum)>
		CL_Shutdown_h;

	extern Hook<void()>
		Dvar_Shutdown_h;

	extern Hook<void()>
		Sys_Quit_h;

	extern Hook<void()>
		CL_Connect_h;

	extern Hook<void(netadr_t from, void* msg)>
		CL_ConnectionlessPacket_h;

	extern Hook<void(netadr_t from)>
		CL_PacketEvent_h;

	extern Hook<void(CoD4XNetadr* from, msg_t* msg)>
		CL_PacketEventCoD4X_h;

	extern Hook<void(const char* remoteName)>
		CL_BeginDownload_h;

	extern Hook<void(int localClientNum, msg_t* msg)>
		CL_ParseGamestate_h;

	extern Hook<void()>
		CL_SystemInfoChanged_h;

	extern Hook<void(int localClientNum)>
		CL_Disconnect_h;

	extern Hook<int(int protocol)>
		CL_RestartForDemo_h;

	extern Hook<void(int localClientNum)>
		CL_ReadDemoMessage_h;

	extern Hook<void FASTCALL(int localClientNum)>
		CL_CreateNewCommands_h;

	extern Hook<void(usercmd_s* cmd)>
		CL_FinishMove_h;

	extern Hook<void FASTCALL(int localClientNum, int key)>
		Message_Key_h;

	extern Hook<int()>
		G_GetFreeCorpseSlot_h;

	extern Hook<void*(int type, const char* name)>
		DB_FindXAssetHeader_h;

	extern Hook<bool(GfxImage* image, void* reader)>
		Image_LoadFromFile_h;

	extern Hook<HRESULT STDCALL(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)>
		IDirect3DDevice9_Reset_h;

	extern Hook<void STDCALL(IDirect3DDevice9* device)>
		IDirect3DDevice9_EndScene_h;

	extern Hook<HMODULE STDCALL(LPCSTR lpLibFileName)>
		LoadLibraryA_h;

	extern Hook<HMODULE STDCALL(LPCWSTR lpLibFileName)>
		LoadLibraryW_h;

	extern Hook<HMODULE STDCALL(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)>
		LoadLibraryExW_h;

	extern Hook<LRESULT CALLBACK(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)>
		MainWndProc_h;

	extern Hook<void STDCALL(int tickRate)>
		PbServerProcessEvents_h;

	API extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_WalkMove_h;

	API extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_AirMove_h;

	API extern Hook<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace_h;

	API extern Hook<void(playerState_s* ps, pml_t* pml)>
		PM_CrashLand_h;

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

	API extern Hook<void()>
		R_BeginFrame_h;

	extern Hook<void()>
		SND_UpdateLoopingSounds_h;

	extern Hook<void(GfxViewInfo* viewInfo)>
		R_ChooseShadowedLights_h;

	extern Hook<void(unsigned int threadContext)>
		CM_InitThreadData_h;

	extern Hook<void(int localClientNum, itemDef_s *item, const char **args)>
		Script_ScriptMenuResponse_h;

	extern Hook<void()>
		UI_VersionNumber_h;

	extern Hook<int(char *dest, size_t size, const char *fmt, va_list va)>
		Vsnprintf_h;

	extern Hook<void()>
		XAssetsInitStdCount_h;

	extern Hook<int(const char* localName, const char* remoteName)>
		DL_BeginDownload_h;

	extern Hook<void()>
		Com_InitDvars_h;

	extern Hook<void()>
		FS_RegisterDvars_h;

	extern Hook<char(saveStatData_t*, const char*)>
		LiveStorage_DecodeStatsData_h;

	extern Hook<void()>
		RB_LookupColor_h;

	extern Hook<void(GfxWindowParms* parms)>
		R_SetWndParms_h;

	extern Hook<void(const refdef_s* refdef)>
		R_RenderScene_h;

	extern Hook<void(int localClientNum)>
		CG_Draw2D_h;

	extern Hook<void()>
		RB_Draw3D_h;

	extern Hook<void(GfxViewInfo* viewInfo)>
		RB_ViewCommands_h;

	extern Hook<bool(DObj_s* obj)>
		CG_UpdateViewModelPose_h;

	extern Hook<bool(DObj_s* obj, const cpose_t* pose, int entnum)>
		R_AddDObjToScene_h;

	extern Hook<bool()>
		CG_Player_h;

	extern Hook<void(int x, int y)>
		UI_MouseEvent_h;

	extern Hook<HRESULT STDCALL(IDirect3DDevice9* device, D3DRENDERSTATETYPE state, DWORD value)>
		IDirect3DDevice9_SetRenderState_h;

	extern Hook<HRESULT STDCALL(IDirect3DDevice9* device, UINT swapChain, UINT index, D3DBACKBUFFER_TYPE type,
		IDirect3DSurface9** surface)>
		IDirect3DDevice9_GetBackBuffer_h;

	extern Hook<HRESULT STDCALL(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
		D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)>
		IDirect3D9_CreateDevice_h;

	extern Function<void(const refdef_s* refdef)>
		R_RenderScene_Original;

	extern Function<void(int localClientNum)>
		CG_Draw2D_Original;

}
// clang-format on
namespace IW3SR
{
	ASM_FUNCTION(CL_Shutdown_h);
	ASM_FUNCTION(CL_ConnectionlessPacket_h);
	ASM_FUNCTION(CL_PacketEvent_h);
	ASM_FUNCTION(CL_BeginDownload_h);
	ASM_FUNCTION(CL_RestartForDemo_h);
	ASM_FUNCTION(CL_RestartForDemoCdecl_h);
	ASM_FUNCTION(CG_Respawn_h);
	ASM_FUNCTION(PM_CrashLand_h);
	ASM_FUNCTION(R_AddCmdDrawText_h);
	ASM_FUNCTION(RB_ExecuteRenderCommandsLoop_h);
	ASM_FUNCTION(RB_LookupColor_h);
	ASM_FUNCTION(MSG_ReadBitsCompress_h);
	ASM_FUNCTION(ParseConfigClient_h);
	ASM_FUNCTION(CL_GetSnapshot_h);
	ASM_FUNCTION(CL_ServerCommand_h);
	ASM_FUNCTION(ExtendedHeader_h);
	ASM_FUNCTION(ReadOriginFloat_h);
	ASM_FUNCTION(DB_FileSize_h);
	ASM_FUNCTION(LoadMapLoadscreen_h);
	ASM_FUNCTION(SetupForNewServerMap_h);
	ASM_FUNCTION(DownloadRate_h);
	ASM_FUNCTION(FrameWait_h);
	ASM_FUNCTION(DSound_UpdateSample_h);
	ASM_FUNCTION(Client_SendVoiceData_h);
	ASM_FUNCTION(CaptureByteRate_h);
	ASM_FUNCTION(R_SetWndParms_h);
	ASM_FUNCTION(R_RenderScene_h);
	ASM_FUNCTION(R_RenderScene_Original);
	ASM_FUNCTION(CG_Draw2D_h);
	ASM_FUNCTION(CG_Draw2D_Original);
	ASM_FUNCTION(RB_ViewCommands_h);
	ASM_FUNCTION(CG_UpdateViewModelPose_h);
	ASM_FUNCTION(R_AddDObjToScene_h);
	ASM_FUNCTION(CG_Player_h);
}
