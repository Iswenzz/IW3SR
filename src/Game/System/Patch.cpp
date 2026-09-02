#include "Patch.hpp"
#include "Autocomplete.hpp"
#include "Huffman.hpp"
#include "PMem.hpp"
#include "Profile.hpp"

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
	#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

namespace IW3SR
{
	// Com_Frame's client spin waits for Sys_Milliseconds to tick over, and sleeps a whole millisecond
	// each time round. What it needs is whatever is left of the current millisecond, so a full one
	// overshoots into the tick after the one it was waiting for and costs roughly a frame in three at
	// com_maxfps 1000. CoD4X rewrites the function and polls at fifty microseconds instead
	constexpr uintptr_t FrameSleepSite = 0x50007F;

	// Negative is relative, in hundreds of nanoseconds.
	constexpr int64_t FrameWaitDue = -500;

	// The download menu draws its transfer rate as bytes divided by the elapsed time, and turns that
	// time into whole seconds before dividing. Everything from the divide by a thousand to the idiv
	// that uses it, replaced by a call that keeps the milliseconds.
	constexpr uintptr_t DownloadRateSite = 0x54A00A;
	constexpr int DownloadRateSize = 0x1A;

	void Patch::Initialize()
	{
		LoadLibraryA_h.Install();
		LoadLibraryW_h.Install();
		LoadLibraryExW_h.Install();

		nlohmann::json settings;
		Environment::Load(settings, "ui.json");
		AllowCoD4X = settings.empty() ? true : settings.value("CoD4X", true);

		if (AllowCoD4X)
			return;

		SkipImproperQuitPrompt();
		SkipOptimalSettingsPrompt();
		WidenColorEscapes();
		DisablePunkbuster();
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

		GPMem::Initialize();

		// Disable <developer 1> condition for debug rendering
		Memory::NOP(0x6496D8, 3);

		// Increase fps cap for menus and loadscreen
		Memory::NOP(0x5001A8, 2);

		// Kill retail's client autoupdate RCE
		Memory::NOP(0x46B8D0, 10);
		Memory::NOP(0x46A919, 5);

		FixDownloadRate();

		RenameConsolePrompt();
		RecolorConsoleText();
		TightenFrameLimiter();

		if (COD4X_VERSION == 213)
			CoD4X_21_3();

		Autocomplete::Initialize();
		GHuffman::Initialize();

		LiveStorage_DecodeStatsData_h.Install();
		Profile::UseCoD4XStatsFormat();

		CreateWindowExA_h.Install();
		Cmd_ExecuteSingleCommand_h.Install();
		Com_InitDvars_h.Install();
		FS_RegisterDvars_h.Install();
		Com_PrintMessage_h.Install();
		CG_CalcViewValues_h.Install();
		CG_DrawCrosshair_h.Install();
		CG_PredictPlayerState_Internal_h.Install();
		CG_RegisterItems_h.Install();
		CG_RegisterWeapons_h.Install();
		CG_Respawn_h.Install();
		CL_InitCGame_h.Install();
		CL_Shutdown_h.Install();
		Dvar_Shutdown_h.Install();
		CL_Connect_h.Install();
		CL_ConnectionlessPacket_h.Install();
		CL_PacketEvent_h.Install();
		CL_BeginDownload_h.Install();
		CL_ParseGamestate_h.Install();
		CL_SystemInfoChanged_h.Install();
		CL_Disconnect_h.Install();
		CL_ReadDemoMessage_h.Install();
		CL_CreateNewCommands_h.Install();
		CL_FinishMove_h.Install();
		DB_LoadXAssets_h.Install();
		DL_BeginDownload_h.Install();
		G_GetFreeCorpseSlot_h.Install();
		DB_FindXAssetHeader_h.Install();
		Image_LoadFromFile_h.Install();
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
		RB_LookupColor_h.Install();
		RB_EndSceneRendering_h.Install();
		Script_ScriptMenuResponse_h.Install();
		UI_VersionNumber_h.Install();
		Vsnprintf_h.Install();
	}

	// A waitable timer sleeps the thread where a spin would hold the core, and the high resolution flag
	// is what gets it under the millisecond the scheduler otherwise rounds up to. Windows before 1803
	// has no such timer and yields instead, which is no worse than the Sleep(1) this replaces. The wait
	// is bounded so a timer that never signals cannot stall the frame.
	void Patch::FrameWait()
	{
		static const HANDLE timer =
			CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

		LARGE_INTEGER due = {};
		due.QuadPart = FrameWaitDue;

		if (!timer || !SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
		{
			Sleep(0);
			return;
		}
		WaitForSingleObject(timer, 1);
	}

	// Truncating the elapsed time to seconds first means the divisor lags the real one by up to a whole
	// second, so the rate reads up to double and only lands on the truth as each second turns over. A
	// download long enough for that to wash out never showed it, which is why it has stood.
	//
	// Guarded on both ends of the block rather than its first byte, since what identifies it is the
	// divide by a thousand at the front and the idiv that consumes the result at the back.
	void Patch::FixDownloadRate()
	{
		if (Memory::Get<uint8_t>(DownloadRateSite) != 0xB8 || Memory::Get<uint32_t>(DownloadRateSite + 1) != 0x10624DD3
			|| Memory::Get<uint16_t>(DownloadRateSite + 0x16) != 0xF9F7)
			return;

		Memory::CALL(DownloadRateSite, ASM_LOAD(DownloadRate_h));
		Memory::NOP(DownloadRateSite + 5, DownloadRateSize - 5);
	}

	void Patch::TightenFrameLimiter()
	{
		// The jump back to the top of the spin moves into the thunk, so the six bytes go as one.
		Memory::JMP(FrameSleepSite, ASM_LOAD(FrameWait_h));
		Memory::NOP(FrameSleepSite + 5, 1);
	}

	// CoD4X's own spin yields where it means to wait. Its usleep is mingw's, which is
	// Sleep(useconds / 1000) - the 0x10624DD3 multiply and shr 6 in the DLL - so usleep(50) is Sleep(0)
	// and the loop spins hot, at the mercy of whatever the scheduler does next. The timer wait costs
	// the same wall clock without holding the core, so its loop gets it too.
	void Patch::TightenFrameLimiterX()
	{
		const uintptr_t site = Signature(COD4X_BIN, "C7 04 24 32 00 00 00 E8");
		if (!site)
		{
			Log::WriteLine(Channel::Error, "The CoD4X frame limiter is not where {} puts it.", COD4X_BIN);
			return;
		}
		// usleep is cdecl and the argument is already written into the frame rather than pushed, so
		// the call goes straight to a function that takes none and the stack is untouched either way.
		Memory::CALL(site + 7, reinterpret_cast<uintptr_t>(&Patch::FrameWait));
	}

	void Patch::DisablePunkbuster()
	{
		// Each loader returns its own error code instead of running.
		const std::pair<uintptr_t, uintptr_t> loaders[] = { { 0x5BF990, 0x6F8D0C }, { 0x5C1230, 0x6F8DDC } };

		for (const auto& [address, error] : loaders)
		{
			Memory::Set<uint8_t>(address, 0xB8);
			Memory::Set<uint32_t>(address + 1, static_cast<uint32_t>(error));
			Memory::Set<uint8_t>(address + 5, 0xC3);
		}

		// jnz to jmp, so startup takes the branch that skips it.
		for (uintptr_t address : { uintptr_t(0x5776C3), uintptr_t(0x5776D6) })
			Memory::Set<uint8_t>(address, 0xEB);
	}

	void Patch::SkipImproperQuitPrompt()
	{
		constexpr uintptr_t site = 0x577415;

		// push 0x33 to a jmp over the prompt.
		Memory::Set<uint8_t>(site, 0xEB);
		Memory::Set<uint8_t>(site + 1, 0x50);
	}

	void Patch::SkipOptimalSettingsPrompt()
	{
		// The call goes, its four arguments come off the stack, and it answers zero.
		for (uintptr_t site : { uintptr_t(0x5766C0), uintptr_t(0x57679A) })
			Memory::Write(site, std::vector<uint8_t>{ 0x83, 0xC4, 0x10, 0x33, 0xC0, 0x90 });
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

		// The short form carries its immediate one byte in, the long form two.
		for (uintptr_t site : sites)
			Memory::Set<uint8_t>(site + (Memory::Get<uint8_t>(site) == 0x3C ? 1 : 2), '@');

		Memory::Set<uint8_t>(spanSite + 1, '@' - '0');
		Memory::Set<uint8_t>(indexSite + 2, '@' - '0' + 1);
	}

	void Patch::RenameConsolePrompt()
	{
		constexpr uintptr_t site = 0x46060E;
		constexpr uintptr_t format = 0x6CF58C;

		static const char prompt[] = "IW3SR> ";

		// The operand of the push that hands the format string to the drawer.
		Memory::Set<uint32_t>(site + 1, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(prompt)));
	}

	void Patch::RecolorConsoleText()
	{
		const auto recolor = [](float* at, const vec4& color) { Memory::Set(reinterpret_cast<uintptr_t>(at), color); };

		recolor(con_versionColor, { 0.0f, 0.9f, 1.0f, 1.0f });
		recolor(con_matchtxtColor_currentDvar, { 0.7f, 0.95f, 1.0f, 1.0f });
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
		TightenFrameLimiterX();
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
		CL_RestartForDemo_h.Update(Signature(COD4X_BIN, "55 57 56 53 89 C3 81 EC 3C 09 00 00"));
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
