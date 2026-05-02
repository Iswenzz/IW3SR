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

		switch (COD4X_VERSION)
		{
		case 211:
			CoD4X_21_1();
			break;
		case 213:
			CoD4X_21_3();
			break;
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
		COD4X_VERSION = GetCoD4XVersion();
	}

	void Patch::CoD4X_21_1()
	{
		// Increase fps cap for menus and loadscreen
		Memory::NOP(Signature(COD4X_BIN, "72 ?? 83 ?? 00 F9 C5 00 07"), 2);

		bg_weaponNames = Signature(0x402D8C).DeRef();
		db_xassetPool = Signature(0x488F05).DeRef();
		g_poolSize = Signature(0x488F0F).DeRef();
		XAssetStdCount = Signature(COD4X_BASE + 0x4482BA0);

		CL_Connect_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? EC 24 04 00 00 E8"));
		CG_Respawn_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 18 B8 ?? ?? ?? ?? 8B 50 20"));
		MainWndProc_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? EC 84 00 00 00 C7 04 24 02"));
		RB_ExecuteRenderCommandsLoop_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 38 89 45 E4 8B 45 E4 89 45 F4"));
		XAssetsInitStdCount_h.Update(COD4X_BASE + 0x82CAF);

		XAssetsInitStdCount_h();
		ReallocXAssetPoolsX();
	}

	void Patch::CoD4X_21_3()
	{
		// Increase fps cap for menus and loadscreen
		Memory::NOP(Signature(COD4X_BIN, "72 ?? 83 ?? 00 F9 C5 00 07"), 2);

		bg_weaponNames = Signature(0x402D8C).DeRef();
		db_xassetPool = Signature(0x488F05).DeRef();
		g_poolSize = Signature(0x488F0F).DeRef();
		XAssetStdCount = Signature(COD4X_BASE + 0x43161C0);

		CL_Connect_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 60 E8 ?? ?? ?? ?? 83 F8 02 74 ?? C7 44 24 04"));
		CL_FinishMove_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 15 ?? ?? ?? ?? 8B 44 24 10 88 50 14 8B 15"));
		CG_Respawn_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? ?? ?? ?? ?? C7 44 24 08 64 2F 00 00 83 C0 0C C7"));
		MainWndProc_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? EC 7C C7 04 24 02 00 00 00"));
		RB_ExecuteRenderCommandsLoop_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 44 24 1C 0F B7 00 8D 5C 24 1C"));
		XAssetsInitStdCount_h.Update(COD4X_BASE + 0x3325E);

		ReallocXAssetPoolsX();
	}

	int Patch::GetCoD4XVersion()
	{
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(COD4X_BASE);
		const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(COD4X_BASE + dos->e_lfanew);
		const char* base = reinterpret_cast<const char*>(COD4X_BASE);
		const size_t size = nt->OptionalHeader.SizeOfImage;

		const std::string_view image{ base, size };
		const std::string_view prefix = "CoD4 MP ";

		const size_t pos = image.find(prefix);
		if (pos == std::string_view::npos)
			return 0;

		const char* versionStart = base + pos + prefix.size();
		const char* versionEnd = base + size;

		std::string versionStr(versionStart, std::find(versionStart, versionEnd, ' '));
		versionStr.erase(std::remove(versionStr.begin(), versionStr.end(), '.'), versionStr.end());

		int version{};
		const auto [ptr, ec] = std::from_chars(versionStr.data(), versionStr.data() + versionStr.size(), version);
		return ec == std::errc{} ? version : 0;
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
