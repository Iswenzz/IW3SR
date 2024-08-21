#include "Patch.hpp"

#include "Core/Memory/Memory.hpp"
#include "Core/Memory/Signature.hpp"
#include "Core/System/System.hpp"

#define COD4X if (COD4X_BASE)

namespace IW3SR
{
	void Patch::Initialize()
	{
		System::MapProcesses();
		System::MapModules(System::Modules[IW3MP_BIN]);

		CoD4X();
		Definitions();
		Renderer();
		Database();
		Player();
		System();
		Hook();
	}

	void Patch::Shutdown()
	{
		Unhook();
	}

	void Patch::Definitions()
	{
		COD4X bg_weaponNames = Signature(COD4X_BASE + 0x443DDE0).DeRef();
		COD4X MainWndProc_h < Signature(COD4X_BIN, "55 89 E5 53 81 EC 84 00 00 00 C7 04 24 02");
		COD4X R_RenderAllLeftovers_h < Signature(COD4X_BIN, "55 89 E5 83 EC 38 89 45 E4 8B 45 E4 89 45 F4");
	}

	void Patch::Renderer()
	{
		// Disable <developer 1> check for debug rendering
		Memory::NOP(0x6496D8, 3);

		// Increase fps cap for menus and loadscreen
		Memory::NOP(0x5001A8, 2);
		//COD4X Memory::NOP(Signature(COD4X_BIN, "72 ?? 83 ?? 00 F9 C5 00 07"), 2);
	}

	void Patch::Database()
	{
		COD4X db_xassetPool = Signature(0x488F05).DeRef();
		COD4X g_poolSize = Signature(0x488F0F).DeRef();
	}

	void Patch::Player()
	{
		COD4X CL_Connect_h < Signature(COD4X_BIN, "55 89 E5 53 81 EC 24 04 00 00 E8");
		COD4X CG_Respawn_h < Signature(COD4X_BIN, "55 89 E5 83 EC 18 B8 ?? ?? ?? ?? 8B 50 20");
	}

	void Patch::System()
	{
		// Increase hunkTotal
		Memory::Set(0x563A29, 0xF0);

		// Increase gmem
		Memory::Set(0x4FF23F, 0x20);
		Memory::Set(0x4FF274, 0x20);
	}

	void Patch::CoD4X()
	{
		auto cod4x = std::ranges::find_if(System::Modules,
			[](const auto& pair) { return pair.first.find("cod4x_") != std::string::npos; });

		if (cod4x == System::Modules.end())
			return;

		COD4X_BIN = cod4x->first;
		COD4X_BASE = cod4x->second;
	}

	void Patch::Hook()
	{
		CreateWindowExA_h.Install();
		Direct3DCreate9_h.Install();
		MainWndProc_h.Install();
		Vsnprintf_h.Install();

		Cmd_ExecuteSingleCommand_h.Install();
		Com_PrintMessage_h.Install();
		CG_DrawCrosshair_h.Install();
		CG_PredictPlayerState_Internal_h.Install();
		COD4X CG_Respawn_h.Install();
		CL_Connect_h.Install();
		CL_Disconnect_h.Install();
		CL_FinishMove_h.Install();
		PM_WalkMove_h.Install();
		PM_AirMove_h.Install();
		PM_GroundTrace_h.Install();
		R_Init_h.Install();
		R_RenderAllLeftovers_h.Install();
		R_Shutdown_h.Install();
		RB_EndSceneRendering_h.Install();
		Script_ScriptMenuResponse_h.Install();
	}

	void Patch::Unhook()
	{
		CreateWindowExA_h.Remove();
		Direct3DCreate9_h.Remove();
		MainWndProc_h.Remove();
		Vsnprintf_h.Remove();

		Cmd_ExecuteSingleCommand_h.Remove();
		Com_PrintMessage_h.Remove();
		CG_DrawCrosshair_h.Remove();
		CG_PredictPlayerState_Internal_h.Remove();
		COD4X CG_Respawn_h.Remove();
		CL_Connect_h.Remove();
		CL_Disconnect_h.Remove();
		CL_FinishMove_h.Remove();
		PM_WalkMove_h.Remove();
		PM_AirMove_h.Remove();
		PM_GroundTrace_h.Remove();
		R_Init_h.Remove();
		R_RenderAllLeftovers_h.Remove();
		R_Shutdown_h.Remove();
		RB_EndSceneRendering_h.Remove();
		Script_ScriptMenuResponse_h.Remove();
	}
}
