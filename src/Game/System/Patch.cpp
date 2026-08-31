#include "Patch.hpp"

namespace IW3SR
{
	void Patch::Initialize()
	{
		LoadLibraryA_h.Install();
		LoadLibraryW_h.Install();
		LoadLibraryExW_h.Install();

		DisableCoD4X = false;
	}

	void Patch::Base()
	{
		if (UseBase)
			return;
		UseBase = true;

		Application::Initialize();

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

		// Kill retail's client autoupdate RCE. A server arms autoupdateStarted (0x8F4CCC) during the
		// handshake, names a file CL_InitDownloads pulls into "updates", and CL_DownloadsComplete then
		// Sys_QuitAndStartProcess-es it
		Memory::NOP(0x46B8D0, 10);
		Memory::NOP(0x46A919, 5);

		// 21.3 is the only build these signatures are written against. Any other keeps the base
		// patches and simply goes without the CoD4X specific retargets.
		if (COD4X_VERSION == 213)
			CoD4X_21_3();

		Autocomplete::Initialize();
		GHuffman::Initialize();

		CreateWindowExA_h.Install();
		Cmd_ExecuteSingleCommand_h.Install();
		Com_InitDvars_h.Install();
		FS_RegisterDvars_h.Install();
		LiveStorage_DecodeStatsData_h.Install();
		Com_PrintMessage_h.Install();
		CG_CalcViewValues_h.Install();
		CG_DrawCrosshair_h.Install();
		CG_PredictPlayerState_Internal_h.Install();
		CG_RegisterItems_h.Install();
		CG_RegisterWeapons_h.Install();
		CG_Respawn_h.Install();
		CL_InitCGame_h.Install();
		CL_Shutdown_h.Install();
		CL_Connect_h.Install();
		CL_Disconnect_h.Install();
		CL_CreateNewCommands_h.Install();
		CL_FinishMove_h.Install();
		DB_LoadXAssets_h.Install();
		G_GetFreeCorpseSlot_h.Install();
		MainWndProc_h.Install();
		PbServerProcessEvents_h.Install();
		PM_WalkMove_h.Install();
		PM_AirMove_h.Install();
		PM_GroundTrace_h.Install();
		PM_CrashLand_h.Install();
		R_AddCmdDrawText_h.Install();
		R_AddCmdDrawTextWithEffects_h.Install();
		R_BeginFrame_h.Install();
		R_Init_h.Install();
		R_Shutdown_h.Install();
		RB_ExecuteRenderCommandsLoop_h.Install();
		RB_EndSceneRendering_h.Install();
		Script_ScriptMenuResponse_h.Install();
		UI_VersionNumber_h.Install();
		Vsnprintf_h.Install();
	}

	void Patch::DisablePunkbuster()
	{
		const std::pair<uintptr_t, uintptr_t> loaders[] = { { 0x5BF990, 0x6F8D0C }, { 0x5C1230, 0x6F8DDC } };

		for (const auto& [address, error] : loaders)
		{
			if (Memory::Get<uint32_t>(address) != 0x0400EC81 || Memory::Get<uint16_t>(address + 4) != 0x0000)
			{
				Log::WriteLine(Channel::Error, "PunkBuster loader at {:X} is not stock 1.7.", address);
				continue;
			}
			Memory::Set<uint8_t>(address, 0xB8);
			Memory::Set<uint32_t>(address + 1, static_cast<uint32_t>(error));
			Memory::Set<uint8_t>(address + 5, 0xC3);
		}
		for (const auto& [address, size] :
			{ std::pair<uintptr_t, uint8_t>{ 0x5776C3, 0x0A }, std::pair<uintptr_t, uint8_t>{ 0x5776D6, 0x19 } })
		{
			if (Memory::Get<uint8_t>(address) != 0x75 || Memory::Get<uint8_t>(address + 1) != size)
			{
				Log::WriteLine(Channel::Error, "PunkBuster startup branch at {:X} is not stock 1.7.", address);
				continue;
			}
			Memory::Set<uint8_t>(address, 0xEB);
		}
	}

	void Patch::SkipImproperQuitPrompt()
	{
		constexpr uintptr_t site = 0x577415;

		if (Memory::Get<uint8_t>(site) != 0x6A || Memory::Get<uint8_t>(site + 1) != 0x33)
		{
			Log::WriteLine(Channel::Error, "The improper quit prompt is not where 1.7 puts it.");
			return;
		}
		Memory::Set<uint8_t>(site, 0xEB);
		Memory::Set<uint8_t>(site + 1, 0x50);
	}

	void Patch::SkipOptimalSettingsPrompt()
	{
		for (uintptr_t site : { uintptr_t(0x5766C0), uintptr_t(0x57679A) })
		{
			if (Memory::Get<uint16_t>(site) != 0x15FF || Memory::Get<uint32_t>(site + 2) != 0x691274)
			{
				Log::WriteLine(Channel::Error, "The optimal settings prompt at {:X} is not stock 1.7.", site);
				continue;
			}
			Memory::Write(site, std::vector<uint8_t>{ 0x83, 0xC4, 0x10, 0x33, 0xC0, 0x90 });
		}
	}

	void Patch::WidenColorEscapes()
	{
		// Two encodings: 3C ii for al, and 80 Fx ii for cl/dl/bl.
		constexpr uintptr_t sites[] = { 0x42D2AA, 0x44B25E, 0x45ED48, 0x53920A, 0x54FC6D, 0x5500C6, 0x558B5E, 0x558BA7,
			0x571D3C, 0x571D86, 0x57A404, 0x5F1F3F, 0x5F1FF6, 0x5F2145, 0x614070 };

		// The visible-length helper at 0x45D520 folds both bounds into one unsigned test instead,
		// 'sub al,0x30 / cmp al,9 / ja reject', so its immediate is the width of the range.
		constexpr uintptr_t spanSite = 0x45D591;

		// Passing the predicate is not enough on its own: the glyph loop then inlines ColorIndex to ask
		// whether the escape is ^7, and retail's version folds every index past 9 to exactly 7. A
		// widened ^: would reset to the base colour instead of reaching the lookup, so the clamp has to
		// move with it - CoD4X takes it to 17 (rb_backend.c:296).
		constexpr uintptr_t indexSite = 0x61407E;

		for (uintptr_t site : sites)
		{
			const bool shortForm = Memory::Get<uint8_t>(site) == 0x3C;
			const uintptr_t operand = site + (shortForm ? 1 : 2);

			if ((!shortForm && Memory::Get<uint8_t>(site) != 0x80) || Memory::Get<uint8_t>(operand) != '9')
			{
				Log::WriteLine(Channel::Error, "The colour escape bound at {:X} is not stock 1.7.", site);
				continue;
			}
			Memory::Set<uint8_t>(operand, '@');
		}

		if (Memory::Get<uint8_t>(spanSite) != 0x3C || Memory::Get<uint8_t>(spanSite + 1) != '9' - '0')
			Log::WriteLine(Channel::Error, "The colour escape span at {:X} is not stock 1.7.", spanSite);
		else
			Memory::Set<uint8_t>(spanSite + 1, '@' - '0');

		if (Memory::Get<uint8_t>(indexSite) != 0x80 || Memory::Get<uint8_t>(indexSite + 2) != '9' - '0' + 1)
			Log::WriteLine(Channel::Error, "The colour index clamp at {:X} is not stock 1.7.", indexSite);
		else
			Memory::Set<uint8_t>(indexSite + 2, '@' - '0' + 1);
	}

	void Patch::RenameConsolePrompt()
	{
		constexpr uintptr_t site = 0x46060E;
		constexpr uintptr_t format = 0x6CF58C;

		static const char prompt[] = "IW3SR> ";

		if (Memory::Get<uint8_t>(site) != 0x68 || Memory::Get<uint32_t>(site + 1) != format)
		{
			Log::WriteLine(Channel::Error, "The console prompt at {:X} is not stock 1.7.", site);
			return;
		}
		Memory::Set<uint32_t>(site + 1, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(prompt)));
	}

	void Patch::RecolorConsoleText()
	{
		constexpr vec4 stockVersion = { 1.0f, 1.0f, 0.0f, 1.0f };
		constexpr vec4 stockMatch = { 1.0f, 1.0f, 0.8f, 1.0f };

		const auto recolor = [](float* at, const vec4& stock, const vec4& color)
		{
			const uintptr_t address = reinterpret_cast<uintptr_t>(at);

			if (Memory::Get<vec4>(address) != stock)
			{
				Log::WriteLine(Channel::Error, "The console text colour at {:X} is not stock 1.7.", address);
				return;
			}
			Memory::Set(address, color);
		};

		recolor(con_versionColor, stockVersion, { 0.0f, 0.9f, 1.0f, 1.0f });
		recolor(con_matchtxtColor_currentDvar, stockMatch, { 0.7f, 0.95f, 1.0f, 1.0f });
	}

	void Patch::CoD4X(HMODULE mod)
	{
		if (UseCoD4X || !mod)
			return;
		UseCoD4X = true;

		char path[MAX_PATH];
		GetModuleFileName(mod, path, MAX_PATH);

		COD4X_BIN = std::filesystem::path(path).filename().string();
		COD4X_BASE = reinterpret_cast<uintptr_t>(mod);
		COD4X_VERSION = GetCoD4XVersion();

		Crash::Patch(COD4X_BASE);
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
		CL_RestartForDemo_h.Update(Signature(COD4X_BIN, "55 89 E5 81 EC 48 09 00 00 C7 04 24"));
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
		s_wmv = Signature(COD4X_BASE + 0x43427C0);

		CL_Connect_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 60 E8 ?? ?? ?? ?? 83 F8 02 74 ?? C7 44 24 04"));
		CL_FinishMove_h.Update(Signature(COD4X_BIN, "?? ?? ?? ?? ?? 15 ?? ?? ?? ?? 8B 44 24 10 88 50 14 8B 15"));
		CL_RestartForDemo_h.Update(Signature(COD4X_BIN, "55 89 E5 81 EC 48 09 00 00 C7 04 24"));
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
			const size_t entrySize = DB_GetXAssetSizeHandlers[type]();
			void* memory = std::calloc(static_cast<size_t>(size), entrySize);

			if (!memory)
			{
				Log::WriteLine(Channel::Error, "Failed to allocate the asset pool for type {}.",
					static_cast<int>(type));
				return;
			}
			const XAssetHeader data = { memory };
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
