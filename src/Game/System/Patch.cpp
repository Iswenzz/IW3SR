#include "Patch.hpp"
#include "CoD4X.hpp"
#include "Autocomplete.hpp"
#include "Huffman.hpp"
#include "PMem.hpp"
#include "Profile.hpp"
#include "Shell.hpp"
#include "Voice.hpp"

#include "Game/Renderer/Materials.hpp"
#include "Game/Renderer/Renderer.hpp"

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
		GShell::GuardCommandLine();

		LoadLibraryA_h.Install();
		LoadLibraryW_h.Install();
		LoadLibraryExW_h.Install();

		nlohmann::json settings;
		Environment::Load(settings, "ui.json");
		const auto cod4x = settings.is_object() ? settings.find("CoD4X") : settings.end();
		AllowCoD4X = cod4x == settings.end() || !cod4x->is_boolean() || cod4x->get<bool>();

		if (AllowCoD4X)
			return;

		DisablePunkbuster();
		SkipImproperQuitPrompt();
		SkipOptimalSettingsPrompt();
		WidenColorEscapes();
	}

	void Patch::Base()
	{
		if (UseBase)
			return;
		UseBase = true;

		Application::Initialize();

		GMaterials::Initialize();
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

		GCoD4X::Install();

		Autocomplete::Initialize();
		GHuffman::Initialize();
		GVoice::Initialize();

		LiveStorage_DecodeStatsData_h.Install();
		Profile::UseCoD4XStatsFormat();

		CreateWindowExA_h.Install();
		Cmd_ExecuteSingleCommand_h.Install();
		Com_InitDvars_h.Install();
		FS_RegisterDvars_h.Install();
		Com_PrintMessage_h.Install();
		CG_CalcViewValues_h.Install();
		CG_CalculateFPS_h.Install();
		CG_DrawCrosshair_h.Install();
		CG_PredictPlayerState_Internal_h.Install();
		CG_RegisterItems_h.Install();
		CG_RegisterWeapons_h.Install();
		CG_Respawn_h.Install();
		CL_InitCGame_h.Install();
		CL_Shutdown_h.Install();
		Dvar_Shutdown_h.Install();
		Sys_Quit_h.Install();
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

	// CoD4X calls the retail loaders and initializers at these same addresses, so the one patch covers
	// both WinMains. What raises MPUI_NOPUNKBUSTER is the initializer answering failure, and CoD4X
	// raises it whenever cl_punkbuster or sv_punkbuster is set, which an archived config leaves on.
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

		// jnz to jmp inside PbClientInitialize and PbServerInitialize, over the failure report and onto
		// the mov al, 1 both of them end on.
		for (uintptr_t address : { uintptr_t(0x5C031F), uintptr_t(0x5C15B8) })
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
		ReallocXAssetPool(XAssetType::ASSET_TYPE_MATERIAL, GMaterials::PoolSize());
		ReallocXAssetPool(XAssetType::ASSET_TYPE_MENU, 1280);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_MENULIST, 256);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_PHYSPRESET, 128);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_STRINGTABLE, 800);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_WEAPON, 2400);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_XANIMPARTS, 8192);
		ReallocXAssetPool(XAssetType::ASSET_TYPE_XMODEL, 5125);
	}
}
