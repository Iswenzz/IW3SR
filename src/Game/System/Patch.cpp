#include "Patch.hpp"

namespace IW3SR
{
	void Patch::Initialize()
	{
		LoadLibraryA_h.Install();
		LoadLibraryW_h.Install();
	}

	void Patch::Base()
	{
		// Increase hunkTotal
		Memory::Set(0x563A29, 0xF0);

		// Increase gmem
		Memory::Set(0x4FF23F, 0x20);
		Memory::Set(0x4FF274, 0x20);

		// Disable <developer 1> condition for debug rendering
		Memory::NOP(0x6496D8, 3);

		// Increase fps cap for menus and loadscreen
		Memory::NOP(0x5001A8, 2);

		CreateWindowExA_h.Install();
		Cmd_ExecuteSingleCommand_h.Install();
		Com_PlayIntroMovies_h.Install();
		Com_PrintMessage_h.Install();
		CG_DrawCrosshair_h.Install();
		CG_PredictPlayerState_Internal_h.Install();
		CG_Respawn_h.Install();
		CL_Connect_h.Install();
		CL_Disconnect_h.Install();
		CL_FinishMove_h.Install();
		MainWndProc_h.Install();
		PM_WalkMove_h.Install();
		PM_AirMove_h.Install();
		PM_GroundTrace_h.Install();
		R_Init_h.Install();
		R_Shutdown_h.Install();
		RB_ExecuteRenderCommandsLoop_h.Install();
		RB_EndSceneRendering_h.Install();
		Script_ScriptMenuResponse_h.Install();
	}

	void Patch::CoD4X(HMODULE mod)
	{
		char path[MAX_PATH];
		GetModuleFileName(mod, path, MAX_PATH);

		COD4X_BIN = std::filesystem::path(path).filename().string();
		COD4X_BASE = reinterpret_cast<uintptr_t>(mod);

		// Increase fps cap for menus and loadscreen
		Memory::NOP(Signature(COD4X_BIN, "72 ?? 83 ?? 00 F9 C5 00 07"), 2);

		bg_weaponNames = Signature(0x402D8C).DeRef();
		db_xassetPool = Signature(0x488F05).DeRef();
		g_poolSize = Signature(0x488F0F).DeRef();

		MainWndProc_h < Signature(COD4X_BIN, "55 89 E5 53 81 EC 84 00 00 00 C7 04 24 02");
		RB_ExecuteRenderCommandsLoop_h < Signature(COD4X_BIN, "55 89 E5 83 EC 38 89 45 E4 8B 45 E4 89 45 F4");
		CL_Connect_h < Signature(COD4X_BIN, "55 89 E5 53 81 EC 24 04 00 00 E8");
		CG_Respawn_h < Signature(COD4X_BIN, "55 89 E5 83 EC 18 B8 ?? ?? ?? ?? 8B 50 20");
	}
}
