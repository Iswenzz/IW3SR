#include "Hooks.hpp"

#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/Renderer/Modules/Settings.hpp"
#include "Game/Renderer/Renderer.hpp"

#include "Game/System/Client.hpp"
#include "Game/System/Console.hpp"
#include "Game/System/System.hpp"

// clang-format off
namespace IW3SR
{
	Hook<HWND STDCALL(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
		DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
		HINSTANCE hInstance, LPVOID lpParam)>
		CreateWindowExA_h(CreateWindowExA, GSystem::CreateMainWindow);

	Hook<void(int localClientNum, int controllerIndex, char* command)>
		Cmd_ExecuteSingleCommand_h(0x4F9AB0, GSystem::ExecuteSingleCommand);

	Hook<void()>
		Com_PlayIntroMovies_h(0x4FEA00, GSystem::Initialize);

	Hook<void(ConChannel channel, const char* msg, int type)>
		Com_PrintMessage_h(0x4FCA50, GConsole::Write);

	Hook<void(int localClientNum)>
		CG_DrawCrosshair_h(0x4311A0, GRenderer::Draw2D);

	Hook<void(int localClientNum)>
		CG_PredictPlayerState_Internal_h(0x447260, Client::Predict);

	Hook<void(int localClientNum)>
		CG_Respawn_h(0x445FA0, ASM_LOAD(CG_Respawn_h));

	Hook<void()>
		CL_Connect_h(0x471050, Client::Connect);

	Hook<void(int localClientNum)>
		CL_Disconnect_h(0x4696B0, Client::Disconnect);

	Hook<void(usercmd_s* cmd)>
		CL_FinishMove_h(0x463A60, PMove::FinishMove);

	Hook<HRESULT STDCALL(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)>
		IDirect3DDevice9_Reset_h(GRenderer::Reset);

	Hook<void STDCALL(IDirect3DDevice9* device)>
		IDirect3DDevice9_EndScene_h(GRenderer::Frame);

	Hook<HMODULE STDCALL(LPCSTR lpLibFileName)>
		LoadLibraryA_h(LoadLibraryA, GSystem::LoadDLL);

	Hook<HMODULE STDCALL(LPCWSTR lpLibFileName)>
		LoadLibraryW_h(LoadLibraryW, GSystem::LoadDLLW);

	Hook<LRESULT CALLBACK(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)>
		MainWndProc_h(0x57BB20, GSystem::MainWndProc);

	Hook<void(pmove_t* pm, pml_t* pml)>
		PM_WalkMove_h(0x40F7A0, PMove::WalkMove);

	Hook<void(pmove_t* pm, pml_t* pml)>
		PM_AirMove_h(0x40F680, PMove::AirMove);

	Hook<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace_h(0x410660, PMove::GroundTrace);

	Hook<void()>
		R_Init_h(0x5F4EE0, GRenderer::Initialize);

	Hook<void(int window)>
		R_Shutdown_h(0x5F4F90, GRenderer::Shutdown);

	Hook<void(GfxImage *image, int samplerIndex, GfxCmdBufSourceState *source, GfxCmdBufState *state, char samplerState)>
		R_SetSampler_h(0x6324B4, ASM_LOAD(R_SetSampler_h));

	Hook<void(void* cmds)>
		RB_ExecuteRenderCommandsLoop_h(0x6156EC, ASM_LOAD(RB_ExecuteRenderCommandsLoop_h));

	Hook<void(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)>
		RB_EndSceneRendering_h(0x6496EC, GRenderer::Draw3D);

	Hook<void(int localClientNum, itemDef_s *item, const char **args)>
		Script_ScriptMenuResponse_h(0x54DD90, GSystem::ScriptMenuResponse);
}
// clang-format on
namespace IW3SR
{
	using namespace asmjit;

	ASM_FUNCTION(CG_Respawn_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.call(ASM_TRAMPOLINE(CG_Respawn_h));

		a.push(x86::dword_ptr(x86::ebp, -0x1C)); // localClientNum
		a.call(Client::Respawn);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_SetSampler_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.movzx(x86::eax, x86::byte_ptr(x86::ebp, 0x10)); // samplerState
		a.push(x86::eax);
		a.push(x86::dword_ptr(x86::ebp, 0x0C));	 // state
		a.push(x86::dword_ptr(x86::ebp, 0x08));	 // source
		a.push(x86::dword_ptr(x86::ebp, -0x1C)); // samplerIndex
		a.push(x86::dword_ptr(x86::ebp, -4));	 // image
		a.call(GRenderer::SetSampler);
		a.add(x86::esp, 20);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(R_SetSampler_h));
	}

	ASM_FUNCTION(RB_ExecuteRenderCommandsLoop_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, -4)); // cmds
		a.call(GRenderer::Commands);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(RB_ExecuteRenderCommandsLoop_h));
	}
}
