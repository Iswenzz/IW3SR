#include "Patch.hpp"

namespace IW3SR
{
	void Patch::Initialize()
	{
		LoadLibraryA_h.Install();
		LoadLibraryW_h.Install();
		LoadLibraryExW_h.Install();
	}

	void Patch::Base()
	{
		if (UseBase)
			return;
		UseBase = true;

		ReallocXAssetPools();

		// Increase hunkTotal
		Memory::Set<uint8_t>(0x563A29, 0xF0);

		// Increase gmem
		Memory::Set<uint8_t>(0x4FF23F, 0x20);
		Memory::Set<uint8_t>(0x4FF274, 0x20);

		// Disable <developer 1> condition for debug rendering
		Memory::NOP(0x6496D8, 3);

		// Increase fps cap for menus and loadscreen
		Memory::NOP(0x5001A8, 2);

		if (UseCoD4X)
		{
			if (CoD4XVer == CoD4XVersion::V21_3)
				CoD4X_21_3();
			else
				CoD4X_21_1();
		}

		CreateWindowExA_h.Install();
		Cmd_ExecuteSingleCommand_h.Install();
		Com_PlayIntroMovies_h.Install();
		Com_PrintMessage_h.Install();
		CG_DrawCrosshair_h.Install();
		CG_PredictPlayerState_Internal_h.Install();
		CG_Respawn_h.Install();
		CL_InitCGame_h.Install();
		CL_Shutdown_h.Install();
		CL_Connect_h.Install();
		CL_Disconnect_h.Install();
		CL_FinishMove_h.Install();
		MainWndProc_h.Install();
		PM_WalkMove_h.Install();
		PM_AirMove_h.Install();
		PM_GroundTrace_h.Install();
		R_AddCmdDrawText_h.Install();
		R_AddCmdDrawTextWithEffects_h.Install();
		R_Init_h.Install();
		R_Shutdown_h.Install();
		RB_ExecuteRenderCommandsLoop_h.Install();
		RB_EndSceneRendering_h.Install();
		Script_ScriptMenuResponse_h.Install();
		UI_VersionNumber_h.Install();
		Vsnprintf_h.Install();
	}

	void Patch::CoD4X(HMODULE mod)
	{
		if (UseCoD4X)
			return;
		UseCoD4X = true;

		char path[MAX_PATH];
		GetModuleFileName(mod, path, MAX_PATH);

		COD4X_BIN = std::filesystem::path(path).filename().string();
		COD4X_BASE = reinterpret_cast<uintptr_t>(mod);

		bg_weaponNames = Signature(0x402D8C).DeRef();
		db_xassetPool = Signature(0x488F05).DeRef();
		g_poolSize = Signature(0x488F0F).DeRef();

		CoD4XVer = DetectCoD4XVersion();
	}

	void Patch::CoD4X_21_1()
	{
		// Increase fps cap for menus and loadscreen
		Memory::NOP(Signature(COD4X_BIN, "72 ?? 83 ?? 00 F9 C5 00 07"), 2);

		XAssetStdCount = Signature(COD4X_BASE + 0x4482BA0);

		MainWndProc_h.Update(Signature(COD4X_BIN, "55 89 E5 53 81 EC 84 00 00 00 C7 04 24 02"));
		RB_ExecuteRenderCommandsLoop_h.Update(Signature(COD4X_BIN, "55 89 E5 83 EC 38 89 45 E4 8B 45 E4 89 45 F4"));
		CL_Connect_h.Update(Signature(COD4X_BIN, "55 89 E5 53 81 EC 24 04 00 00 E8"));
		CG_Respawn_h.Update(Signature(COD4X_BIN, "55 89 E5 83 EC 18 B8 ?? ?? ?? ?? 8B 50 20"));
		XAssetsInitStdCount_h.Update(Signature(COD4X_BASE + 0x82CAF));

		WriteXAssetStdCounts();
	}

	void Patch::CoD4X_21_3()
	{
		// Increase fps cap for menus and loadscreen
		Memory::NOP(Signature(COD4X_BIN, "72 ?? 83 3D 00 F9 C5 00 07"), 2);

		XAssetStdCount = Signature(COD4X_BASE + 0x43161C0);

		MainWndProc_h.Update(Signature(COD4X_BIN, "55 57 56 53 83 EC 7C C7 04 24 02 00 00 00"));
		CL_Connect_h.Update(Signature(COD4X_BIN, "57 56 53 83 EC 60 E8 ?? ?? ?? ?? 83 F8 02 74 ?? C7 44 24 04 ?? ?? ?? ?? C7 04 24"));
		CG_Respawn_h.Update(Signature(COD4X_BIN, "57 83 EC 18 A1 ?? ?? ?? ?? C7 44 24 08 64 2F 00 00 83 C0 0C C7 04 24"));
		RB_ExecuteRenderCommandsLoop_h.Update(Signature(COD4X_BIN, "53 83 EC 28 89 44 24 1C 0F B7 00 8D 5C 24 1C 66 85 C0 74 1D"));
		CL_FinishMove_h.Update(Signature(COD4X_BIN, "53 83 EC 08 8B 15 ?? ?? ?? ?? 8B 44 24 10 88 50 14 8B 15 ?? ?? ?? ?? 88 50 15"));
		XAssetsInitStdCount_h.Update(COD4X_BASE + 0x3325E);

		WriteXAssetStdCounts();
	}

	CoD4XVersion Patch::DetectCoD4XVersion()
	{
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(COD4X_BASE);
		const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(COD4X_BASE + dos->e_lfanew);
		const char* base = reinterpret_cast<const char*>(COD4X_BASE);
		const size_t size = nt->OptionalHeader.SizeOfImage;
		const std::string_view needle = "CoD4 MP 21.3";

		for (size_t i = 0; i + needle.size() <= size; ++i)
		{
			if (std::memcmp(base + i, needle.data(), needle.size()) == 0)
				return CoD4XVersion::V21_3;
		}
		return CoD4XVersion::V21_1;
	}

	void Patch::ReallocXAssetPools()
	{
		const auto ReallocXAssetPool = [](XAssetType type, int size)
		{
			const XAssetHeader data = { std::calloc(1, size * DB_GetXAssetSizeHandlers[type]()) };
			db_xassetPool[type] = data;
			g_poolSize[type] = size;
		};
		ReallocXAssetPool(XAssetType::ASSET_TYPE_FX, 1200);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_GAMEWORLD_SP, 1);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_IMAGE, 7168);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_LOADED_SOUND, 2700);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_LOCALIZE_ENTRY, 14000);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_MATERIAL, 8192);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_MENU, 1280);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_MENULIST, 256);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_PHYSPRESET, 128);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_STRINGTABLE, 800);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_WEAPON, 2400);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_XANIMPARTS, 8192);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_XMODEL, 5125);
	}

	void Patch::ReallocXAssetPoolsX()
	{
		XAssetsInitStdCount_h();
		WriteXAssetStdCounts();
	}

	void Patch::WriteXAssetStdCounts()
	{
		XAssetStdCount[XAssetType::ASSET_TYPE_FX] = 1200;
		XAssetStdCount[XAssetType::ASSET_TYPE_GAMEWORLD_SP] = 1;
		XAssetStdCount[XAssetType::ASSET_TYPE_IMAGE] = 7168;
		XAssetStdCount[XAssetType::ASSET_TYPE_LOADED_SOUND] = 2700;
		XAssetStdCount[XAssetType::ASSET_TYPE_LOCALIZE_ENTRY] = 14000;
		XAssetStdCount[XAssetType::ASSET_TYPE_MATERIAL] = 8192;
		XAssetStdCount[XAssetType::ASSET_TYPE_MENU] = 1280;
		XAssetStdCount[XAssetType::ASSET_TYPE_MENULIST] = 256;
		XAssetStdCount[XAssetType::ASSET_TYPE_PHYSPRESET] = 128;
		XAssetStdCount[XAssetType::ASSET_TYPE_STRINGTABLE] = 800;
		XAssetStdCount[XAssetType::ASSET_TYPE_WEAPON] = 2400;
		XAssetStdCount[XAssetType::ASSET_TYPE_XANIMPARTS] = 8192;
		XAssetStdCount[XAssetType::ASSET_TYPE_XMODEL] = 5125;
	}
}
