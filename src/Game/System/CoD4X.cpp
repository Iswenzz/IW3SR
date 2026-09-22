#include "CoD4X.hpp"
#include "Patch.hpp"

#include "Game/Renderer/Renderer.hpp"

namespace IW3SR
{
	constexpr int TestedVersions[] = { 213, 214, 215 };
	constexpr int MinimumVersion = 213;
	constexpr float UntestedNoticeDelay = 5.0f;
	constexpr float UntestedNoticeDuration = 20.0f;

	constexpr uintptr_t XAssetStdCountOperand = 2;
	constexpr uintptr_t WinMouseVarsOperand = 23;

	constexpr const char* MenuFpsCapSignature = "72 ?? 83 ?? 00 F9 C5 00 07";
	constexpr const char* FrameLimiterSignature = "C7 04 24 32 00 00 00 E8";
	constexpr const char* ConnectSignature = "?? ?? ?? ?? ?? 60 E8 ?? ?? ?? ?? 83 F8 02 74 ?? C7 44 24 04";
	constexpr const char* FinishMoveSignature = "?? ?? ?? ?? ?? 15 ?? ?? ?? ?? 8B 44 24 10 88 50 14 8B 15";
	constexpr const char* RespawnSignature = "?? ?? ?? ?? ?? ?? ?? ?? ?? C7 44 24 08 64 2F 00 00 83 C0 0C C7";
	constexpr const char* RenderCommandsSignature = "?? ?? ?? ?? ?? 44 24 1C 0F B7 00 8D 5C 24 1C";
	constexpr const char* QuitSignature =
		"83 EC 1C A1 ?? ?? ?? ?? 83 C0 30 89 04 24 FF 15 ?? ?? ?? ?? 83 EC 04 C7 04 24 01 00 00 00 FF 15";
	constexpr const char* XAssetsInitStdCountSignature =
		"C7 05 ?? ?? ?? ?? 40 00 00 00 C7 05 ?? ?? ?? ?? 40 00 00 00 C7 05 ?? ?? ?? ?? 00 10 00 00";
	constexpr const char* WinMouseVarsSignature =
		"89 15 ?? ?? ?? ?? 89 54 24 04 FF 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? C6 05 ?? ?? ?? ?? 01";
	constexpr const char* MainWndProcSignature = "55 57 56 53 83 EC 7C 8B AC 24 90 00 00 00";
	constexpr const char* MainWndProcSignatureEax = "?? ?? ?? ?? ?? EC 7C C7 04 24 02 00 00 00";
	constexpr const char* RestartForDemoSignature =
		"55 57 56 53 81 EC 4C 09 00 00 C7 04 24 ?? ?? ?? ?? 8B 9C 24 60 09 00 00";
	constexpr const char* RestartForDemoSignatureEax = "55 57 56 53 89 C3 81 EC 3C 09 00 00";

	void GCoD4X::Attach(HMODULE mod)
	{
		if (!mod || reinterpret_cast<uintptr_t>(mod) == COD4X_BASE)
			return;
		Patch::UseCoD4X = true;

		char path[MAX_PATH];
		GetModuleFileName(mod, path, MAX_PATH);

		COD4X_BIN = std::filesystem::path(path).filename().string();
		COD4X_BASE = reinterpret_cast<uintptr_t>(mod);
		COD4X_VERSION = ReadVersion();

		Crash::Patch(COD4X_BASE);
		TightenFrameLimiter();
	}

	void GCoD4X::Install()
	{
		if (!COD4X_BASE)
			return;

		WarnUntested();
		if (COD4X_VERSION < MinimumVersion)
			return;

		// Increase fps cap for menus and loadscreen
		Memory::NOP(Signature(COD4X_BIN, MenuFpsCapSignature), 2);

		bg_weaponNames = Signature(0x402D8C).DeRef();
		db_xassetPool = Signature(0x488F05).DeRef();
		g_poolSize = Signature(0x488F0F).DeRef();
		s_wmv = FindWinMouseVars();

		CL_Connect_h.Update(Signature(COD4X_BIN, ConnectSignature));
		CL_FinishMove_h.Update(Signature(COD4X_BIN, FinishMoveSignature));
		CL_RestartForDemo_h.Update(FindRestartForDemo());
		CG_Respawn_h.Update(Signature(COD4X_BIN, RespawnSignature));
		MainWndProc_h.Update(FindMainWndProc());
		RB_ExecuteRenderCommandsLoop_h.Update(Signature(COD4X_BIN, RenderCommandsSignature));
		Sys_Quit_h.Update(Signature(COD4X_BIN, QuitSignature));
		XAssetsInitStdCount_h.Update(FindXAssetsInitStdCount());

		ReportMisses();
		ReallocXAssetPools();
	}

	uintptr_t GCoD4X::FindMainWndProc()
	{
		if (const uintptr_t address = Signature(COD4X_BIN, MainWndProcSignature))
			return address;
		return Signature(COD4X_BIN, MainWndProcSignatureEax);
	}

	uintptr_t GCoD4X::FindRestartForDemo()
	{
		if (const uintptr_t address = Signature(COD4X_BIN, RestartForDemoSignature))
		{
			CL_RestartForDemo_h.Callback = ASM_LOAD(CL_RestartForDemoCdecl_h);
			return address;
		}
		return Signature(COD4X_BIN, RestartForDemoSignatureEax);
	}

	uintptr_t GCoD4X::FindXAssetsInitStdCount()
	{
		const uintptr_t address = Signature(COD4X_BIN, XAssetsInitStdCountSignature);

		XAssetStdCount = address ? *reinterpret_cast<int**>(address + XAssetStdCountOperand) : nullptr;
		return address;
	}

	WinMouseVars_t* GCoD4X::FindWinMouseVars()
	{
		const uintptr_t address = Signature(COD4X_BIN, WinMouseVarsSignature);
		if (!address)
			return nullptr;

		auto* field = *reinterpret_cast<uint8_t**>(address + WinMouseVarsOperand);
		return reinterpret_cast<WinMouseVars_t*>(field - offsetof(WinMouseVars_t, mouseInitialized));
	}

	void GCoD4X::TightenFrameLimiter()
	{
		const uintptr_t site = Signature(COD4X_BIN, FrameLimiterSignature);
		if (!site)
		{
			Log::WriteLine(Channel::Error, "The CoD4X frame limiter is not where {} puts it.", COD4X_BIN);
			return;
		}
		Memory::CALL(site + 7, reinterpret_cast<uintptr_t>(&Patch::FrameWait));
	}

	void GCoD4X::ReportMisses()
	{
		const std::pair<const char*, bool> resolved[] = {
			{ "CL_Connect", CL_Connect_h.IsEnabled },
			{ "CL_FinishMove", CL_FinishMove_h.IsEnabled },
			{ "CL_FindAndRunOldVersion2", CL_RestartForDemo_h.IsEnabled },
			{ "CG_Respawn", CG_Respawn_h.IsEnabled },
			{ "MainWndProc", MainWndProc_h.IsEnabled },
			{ "RB_ExecuteRenderCommandsLoop", RB_ExecuteRenderCommandsLoop_h.IsEnabled },
			{ "Sys_Quit", Sys_Quit_h.IsEnabled },
			{ "XAssetsInitStdCount", XAssetsInitStdCount_h.IsEnabled },
			{ "s_wmv", s_wmv != nullptr },
			{ "XAssetStdCount", XAssetStdCount != nullptr },
		};

		for (const auto& [name, found] : resolved)
			if (!found)
				Log::WriteLine(Channel::Error, "CoD4X {}: {} did not resolve.", FormatVersion(COD4X_VERSION), name);
	}

	void GCoD4X::WarnUntested()
	{
		if (std::ranges::contains(TestedVersions, COD4X_VERSION))
			return;

		std::string tested;
		for (int version : TestedVersions)
			tested += (tested.empty() ? "" : ", ") + FormatVersion(version);

		const std::string message = std::format("CoD4X {} has not been tested with IW3SR.\nTested releases are {}.",
			FormatVersion(COD4X_VERSION), tested);

		Log::WriteLine(Channel::Warning, "{}", message);
		GRenderer::Tasks.Add([message]()
			{ Notifications::Push(message, NotificationLevel::Warning, UntestedNoticeDuration, UntestedNoticeDelay); });
	}

	int GCoD4X::ReadVersion()
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

	std::string GCoD4X::FormatVersion(int version)
	{
		if (version <= 0)
			return "(unknown version)";

		const std::string digits = std::to_string(version);
		return digits.size() < 2 ? digits : digits.substr(0, digits.size() - 1) + "." + digits.back();
	}

	void GCoD4X::ReallocXAssetPools()
	{
		if (!XAssetStdCount)
			return;

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
