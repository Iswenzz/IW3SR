#include "Hooks.hpp"

#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/Renderer/Portal/Portal.hpp"
#include "Game/Renderer/Renderer.hpp"

#include "Game/System/Assets.hpp"
#include "Game/System/Capture.hpp"
#include "Game/System/Channel.hpp"
#include "Game/System/Client.hpp"
#include "Game/System/Console.hpp"
#include "Game/System/Colors.hpp"
#include "Game/System/Demo.hpp"
#include "Game/System/Download.hpp"
#include "Game/System/Huffman.hpp"
#include "Game/System/PMem.hpp"
#include "Game/System/Profile.hpp"
#include "Game/System/Patch.hpp"
#include "Game/System/Protocol.hpp"
#include "Game/System/Server.hpp"
#include "Game/System/System.hpp"
#include "Game/System/Timestep.hpp"
#include "Game/System/Zones.hpp"

#include <cstddef>

// clang-format off
namespace IW3SR
{
	Hook<HWND STDCALL(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
		DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
		HINSTANCE hInstance, LPVOID lpParam)>
		CreateWindowExA_h(CreateWindowExA, GSystem::CreateMainWindow);

	Hook<void(int localClientNum, int controllerIndex, char* command)>
		Cmd_ExecuteSingleCommand_h(0x4F9AB0, GSystem::ExecuteSingleCommand);

	Hook<void(ConChannel channel, const char* msg, int type)>
		Com_PrintMessage_h(0x4FCA50, GConsole::Write);

	Hook<void(int localClientNum)>
		CG_CalcViewValues_h(0x451990, Timestep::CalcViewValues);

	Hook<void(int localClientNum)>
		CG_DrawCrosshair_h(0x4311A0, GRenderer::Draw2D);

	Hook<void(int localClientNum)>
		CG_PredictPlayerState_Internal_h(0x447260, Client::Predict);

	Hook<void()>
		CG_RegisterItems_h(0x454BA0, Assets::RegisterItems);

	Hook<void(const char** weapons, int weaponCount)>
		CG_RegisterWeapons_h(0x458E20, Assets::RegisterWeapons);

	Hook<void(int localClientNum)>
		CG_Respawn_h(0x445FA0, ASM_LOAD(CG_Respawn_h));

	Hook<void(int localClientNum)>
		CL_InitCGame_h(0x45BEF0, Client::Initialize);

	Hook<void(XZoneInfo* zoneInfo, unsigned int zoneCount, int sync)>
		DB_LoadXAssets_h(0x48A2B0, Assets::LoadXAssets);

	Hook<void(int localClientNum)>
		CL_Shutdown_h(0x46FDF0, ASM_LOAD(CL_Shutdown_h));

	Hook<void()>
		Dvar_Shutdown_h(0x56B7D0, Dvar::Shutdown);

	Hook<void()>
		CL_Connect_h(0x471050, Client::Connect);

	Hook<void(netadr_t from, void* msg)>
		CL_ConnectionlessPacket_h(0x46C0D0, ASM_LOAD(CL_ConnectionlessPacket_h));

	Hook<void(netadr_t from)>
		CL_PacketEvent_h(0x46C320, ASM_LOAD(CL_PacketEvent_h));

	Hook<void(const char* remoteName)>
		CL_BeginDownload_h(0x46AB00, ASM_LOAD(CL_BeginDownload_h));

	Hook<void(int localClientNum, msg_t* msg)>
		CL_ParseGamestate_h(0x473CE0, GProtocol::ParseGamestateHook);

	Hook<void()>
		CL_SystemInfoChanged_h(0x473AB0, GProtocol::SystemInfoChanged);

	Hook<void(int localClientNum)>
		CL_Disconnect_h(0x4696B0, Client::Disconnect);

	Hook<int(int protocol)>
		CL_RestartForDemo_h(uintptr_t(0), ASM_LOAD(CL_RestartForDemo_h));

	Hook<void(int localClientNum)>
		CL_ReadDemoMessage_h(0x4690C0, Demo::ReadMessage);

	Hook<void FASTCALL(int localClientNum)>
		CL_CreateNewCommands_h(0x463E00, Timestep::CreateNewCommands);

	Hook<void(usercmd_s* cmd)>
		CL_FinishMove_h(0x463A60, PMove::FinishMove);

	Hook<int()>
		G_GetFreeCorpseSlot_h(0x4C9770, GServer::GetFreeCorpseSlot);

	Hook<void*(int type, const char* name)>
		DB_FindXAssetHeader_h(0x489570, Assets::FindXAssetHeader);

	Hook<bool(GfxImage* image, void* reader)>
		Image_LoadFromFile_h(0x642380, Assets::LoadImageFromFile);

	Hook<HRESULT STDCALL(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)>
		IDirect3DDevice9_Reset_h(GRenderer::Reset);

	Hook<void STDCALL(IDirect3DDevice9* device)>
		IDirect3DDevice9_EndScene_h(GRenderer::Frame);

	Hook<HMODULE STDCALL(LPCSTR lpLibFileName)>
		LoadLibraryA_h(LoadLibraryA, GSystem::LoadDLL);

	Hook<HMODULE STDCALL(LPCWSTR lpLibFileName)>
		LoadLibraryW_h(LoadLibraryW, GSystem::LoadDLLW);

	Hook<HMODULE STDCALL(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)>
		LoadLibraryExW_h(LoadLibraryExW, GSystem::LoadDLLExW);

	Hook<LRESULT CALLBACK(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)>
		MainWndProc_h(0x57BB20, GSystem::MainWndProc);

	Hook<void STDCALL(int tickRate)>
		PbServerProcessEvents_h(0x5C13C0, GSystem::MainLoop);

	Hook<void(pmove_t* pm, pml_t* pml)>
		PM_WalkMove_h(0x40F7A0, PMove::WalkMove);

	Hook<void(pmove_t* pm, pml_t* pml)>
		PM_AirMove_h(0x40F680, PMove::AirMove);

	Hook<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace_h(0x410660, PMove::GroundTrace);

	Hook<void(playerState_s* ps, pml_t* pml)>
		PM_CrashLand_h(0x40FFB0, ASM_LOAD(PM_CrashLand_h));

	Hook<int(const char* localName, const char* remoteName)>
		DL_BeginDownload_h(0x500AE0, GDownload::BeginDownload);

	Hook<void()>
		Com_InitDvars_h(0x4FEA80, GPMem::InitDvars);

	Hook<void()>
		FS_RegisterDvars_h(0x55E390, Profile::RegisterDvars);

	Hook<char(saveStatData_t*, const char*)>
		LiveStorage_DecodeStatsData_h(0x579540, Profile::DecodeStats);

	Hook<void()>
		RB_LookupColor_h(0x613790, ASM_LOAD(RB_LookupColor_h));

	Hook<void(const char** text, int maxChars, Font_s* font, float x, float y, float xScale, float yScale, float rotation,
		int style, const vec4& color)>
		R_AddCmdDrawText_h(0x5F6B00, ASM_LOAD(R_AddCmdDrawText_h));

	Hook<void(const char* text, int maxChars, Font_s* font, float x, float y, float xScale, float yScale, float rotation,
		const vec4& color, int style, const vec4& glowColor, Material* fxMaterial, Material* fxMaterialGlow,
		int fxBirthTime, int fxLetterTime, int fxDecayStartTime, int fxDecayDuration)>
		R_AddCmdDrawTextWithEffects_h(0x5F6D30, GRenderer::AddCmdDrawTextWithEffects);

	Hook<void()>
		R_Init_h(0x5F4EE0, GRenderer::Initialize);

	Hook<void(int window)>
		R_Shutdown_h(0x5F4F90, GRenderer::Shutdown);

	Hook<void(void* cmds)>
		RB_ExecuteRenderCommandsLoop_h(0x6156EC, ASM_LOAD(RB_ExecuteRenderCommandsLoop_h));

	Hook<void(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)>
		RB_EndSceneRendering_h(0x6496EC, GRenderer::Draw3D);

	Hook<void()>
		R_BeginFrame_h(0x5F75A0, GPortal::BeginFrame);

	Hook<void(int localClientNum, itemDef_s *item, const char **args)>
		Script_ScriptMenuResponse_h(0x54DD90, GSystem::ScriptMenuResponse);

	Hook<void()>
		UI_VersionNumber_h(0x543410, GRenderer::DrawVersion);

	Hook<int(char *dest, size_t size, const char *fmt, va_list va)>
		Vsnprintf_h(0x6706F5, GSystem::Vsnprintf);

	Hook<void()>
		XAssetsInitStdCount_h(Patch::ReallocXAssetPoolsX);
}
// clang-format on
namespace IW3SR
{
	using namespace asmjit;

	ASM_FUNCTION(CL_Shutdown_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, -0x04)); // (eax) localClientNum
		a.call(GSystem::Shutdown);
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(CL_Shutdown_h));
	}

	ASM_FUNCTION(CL_ConnectionlessPacket_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::eax, x86::dword_ptr(x86::ebp, -0x04)); // (eax) msg
		a.mov(x86::eax, x86::dword_ptr(x86::eax, offsetof(msg_t, data))); // msg->data
		a.add(x86::eax, 0x04);
		a.push(x86::eax); // packet, past the 0xFFFFFFFF marker
		a.lea(x86::eax, x86::dword_ptr(x86::ebp, 0x08));
		a.push(x86::eax); // from
		a.call(GProtocol::Inspect);
		a.add(x86::esp, 0x08);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(CL_ConnectionlessPacket_h));
	}

	ASM_FUNCTION(CL_PacketEvent_h)
	{
		Label consumed = a.newLabel();

		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, -0x0C)); // (edx) time
		a.push(x86::dword_ptr(x86::ebp, -0x1C)); // (esi) msg
		a.lea(x86::eax, x86::dword_ptr(x86::ebp, 0x08));
		a.push(x86::eax); // from
		a.call(GChannel::PacketEvent);
		a.add(x86::esp, 0x0C);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax); // returned through popad's eax slot

		a.popad();
		a.pop(x86::ebp);

		a.test(x86::eax, x86::eax);
		a.jnz(consumed);
		a.jmp(ASM_TRAMPOLINE(CL_PacketEvent_h));

		a.bind(consumed);
		a.ret();
	}

	ASM_FUNCTION(CL_RestartForDemo_h)
	{
		Label restart = a.newLabel();

		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, -0x04)); // (eax) protocol
		a.call(Capture::RestartForDemo);
		a.add(x86::esp, 0x04);

		a.test(x86::eax, x86::eax);
		a.jnz(restart);

		a.popad();
		a.pop(x86::ebp);
		a.xor_(x86::eax, x86::eax);
		a.ret();

		a.bind(restart);
		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(CL_RestartForDemo_h));
	}

	ASM_FUNCTION(CL_BeginDownload_h)
	{
		Label allowed = a.newLabel();

		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x08));	 // remoteName
		a.push(x86::dword_ptr(x86::ebp, -0x04)); // (eax) localName
		a.call(GDownload::AllowBegin);
		a.add(x86::esp, 0x08);

		a.test(x86::eax, x86::eax);
		a.jnz(allowed);

		a.popad();
		a.pop(x86::ebp);
		a.ret();

		a.bind(allowed);
		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(CL_BeginDownload_h));
	}

	ASM_FUNCTION(CG_Respawn_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.call(ASM_TRAMPOLINE(CG_Respawn_h));

		a.push(x86::dword_ptr(x86::ebp, -0x1C)); // (esi) localClientNum
		a.call(Client::Respawn);
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_CrashLand_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x08));	 // pml
		a.push(x86::dword_ptr(x86::ebp, -0x1C)); // (esi) ps
		a.call(PMove::CrashLand);
		a.add(x86::esp, 0x08);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(PM_CrashLand_h));
	}

	ASM_FUNCTION(R_AddCmdDrawText_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, -0x08)); // (ecx) color
		a.push(x86::dword_ptr(x86::ebp, 0x28));	 // style
		a.push(x86::dword_ptr(x86::ebp, 0x24));	 // rotation
		a.push(x86::dword_ptr(x86::ebp, 0x20));	 // yScale
		a.push(x86::dword_ptr(x86::ebp, 0x1C));	 // xScale
		a.push(x86::dword_ptr(x86::ebp, 0x18));	 // y
		a.push(x86::dword_ptr(x86::ebp, 0x14));	 // x
		a.push(x86::dword_ptr(x86::ebp, 0x10));	 // font
		a.push(x86::dword_ptr(x86::ebp, 0x0C));	 // maxChars
		a.lea(x86::eax, x86::dword_ptr(x86::ebp, 0x08));
		a.push(x86::eax); // text
		a.call(GRenderer::AddCmdDrawText);
		a.add(x86::esp, 0x28);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(R_AddCmdDrawText_h));
	}

	ASM_FUNCTION(RB_LookupColor_h)
	{
		a.pushad();
		a.push(x86::edx);
		a.movzx(x86::eax, x86::cl);
		a.push(x86::eax);
		a.call(Colors::Lookup);
		a.add(x86::esp, 0x08);
		a.popad();
		a.ret();
	}

	ASM_FUNCTION(RB_ExecuteRenderCommandsLoop_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, -0x04)); // (eax) cmds
		a.call(GRenderer::ExecuteRenderCommandsLoop);
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.jmp(ASM_TRAMPOLINE(RB_ExecuteRenderCommandsLoop_h));
	}

	ASM_FUNCTION(MSG_ReadBitsCompress_h)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);

		a.push(x86::dword_ptr(x86::ebp, 0x0C)); // readsize
		a.push(x86::dword_ptr(x86::ebp, 0x08)); // output
		a.push(x86::eax);						// input
		a.mov(x86::ecx, reinterpret_cast<uintptr_t>(&GHuffman::Decompress));
		a.call(x86::ecx);

		a.mov(x86::esp, x86::ebp);
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(ParseConfigClient_h)
	{
		a.lea(x86::eax, x86::dword_ptr(x86::esp, 0x10));
		a.pushad();
		a.push(x86::eax);
		a.call(GProtocol::ParseConfigClient);
		a.add(x86::esp, 0x04);
		a.popad();

		a.push(imm(0x474838));
		a.ret();
	}

	ASM_FUNCTION(CL_GetSnapshot_h)
	{
		Label empty = a.newLabel();

		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);

		a.push(x86::dword_ptr(x86::ebp, 0x0C)); // snapshot
		a.push(x86::dword_ptr(x86::ebp, 0x08)); // snapshotNumber
		a.mov(x86::edx, 0x45AB90);
		a.call(x86::edx);
		a.add(x86::esp, 0x08);

		a.test(x86::eax, x86::eax);
		a.jz(empty);

		a.pushad();
		a.push(x86::dword_ptr(x86::ebp, 0x0C));
		a.call(GProtocol::ApplySnapshotNames);
		a.add(x86::esp, 0x04);
		a.popad();

		a.bind(empty);
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(CL_ServerCommand_h)
	{
		a.mov(x86::byte_ptr(x86::esi, 0x3FF), imm(0));

		a.pushad();
		a.push(x86::esi);
		a.call(GProtocol::ServerCommand);
		a.add(x86::esp, 0x04);
		a.popad();

		a.ret();
	}

	ASM_FUNCTION(ExtendedHeader_h)
	{
		const uintptr_t fields[] = { 0xC84FE4, 0x914E1C, 0x914E20,
			reinterpret_cast<uintptr_t>(&ExtendedConfigDataSequence) };

		a.push(x86::ebx);
		a.push(x86::ebp);
		a.push(x86::esi);
		a.push(x86::edi);

		for (uintptr_t field : fields)
		{
			a.mov(x86::eax, x86::esi);
			a.mov(x86::edi, x86::dword_ptr(field));
			a.mov(x86::edx, 0x5054A0);
			a.call(x86::edx);
		}

		a.pop(x86::edi);
		a.pop(x86::esi);
		a.pop(x86::ebp);
		a.pop(x86::ebx);
		a.ret();
	}

	ASM_FUNCTION(ReadOriginFloat_h)
	{
		a.push(x86::eax);
		a.call(GProtocol::ReadOriginFloat);
		a.add(x86::esp, 0x04);
		a.ret();
	}

	ASM_FUNCTION(DB_FileSize_h)
	{
		a.push(x86::eax);							  // the name, for the second call
		a.push(x86::dword_ptr(x86::esp, 0x08));		  // the path kind, back where the original wants it
		a.mov(x86::edx, 0x48B940);
		a.call(x86::edx);
		a.add(x86::esp, 0x04);
		a.pop(x86::ecx);							  // the name again; the original may have clobbered it

		a.push(x86::eax);							  // the size it found, zero when it found nothing
		a.push(x86::ecx);
		a.call(GZones::FileSize);
		a.add(x86::esp, 0x08);
		a.ret();
	}
}
