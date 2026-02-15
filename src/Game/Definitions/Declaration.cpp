#include "Declaration.hpp"

namespace IW3SR
{
	// Binaries
	std::string COD4X_BIN = "";
	std::string IW3MP_BIN = "iw3mp.exe";
	std::string IW3XO_BIN = "iw3xo.dll";

	// Base
	uintptr_t COD4X_BASE = 0;
	uintptr_t IW3MP_BASE = uintptr_t(GetModuleHandle(IW3MP_BIN.c_str()));
	uintptr_t IW3XO_BASE = uintptr_t(GetModuleHandle(IW3XO_BIN.c_str()));

	// Game
	CmdArgs* cmd_args = Signature(0x1410B40);
	cmd_function_s** cmds = Signature(0x1410B3C);
	clientActive_t* clients = Signature(0xC5F930);
	clientStatic_t* cls = Signature(0x956D80);
	clientConnection_t& clc = Signature(0x8F4CE0);
	cg_s* cgs = Signature(0x74E338);
	centity_s* cg_entities = Signature(0x84F2D8);
	int& com_frameTime = Signature(0x1476EFC);
	float& com_timescaleValue = Signature(0x1435D68);
	int* g_entities = Signature(0x12885C4);
	int* g_clients = Signature(0x13255A8);
	pmove_t* pmove = Signature(0x8C9C90);
	playerState_s* ps_loc = Signature(0x13255A8);

	const char* g_entityBeginParsePoint = Signature(0x1113674);
	const char* g_entityEndParsePoint = Signature(0x1113678);
	int* client_activeNumber = Signature(0xCC5FF8);
	infoParm_t* info_params = Signature(0x71FBD0);
	WeaponDef** bg_weaponNames = Signature(0x736DB8);
	dvar_s** dvars = Signature(0xCBA7408);
	int& dvarCount = Signature(0xCBA73F8);
	int* server_fps = Signature(0xCBAB908);

	// Render
	DxGlobals* dx = Signature(0xCC9A400);
	MaterialLoadGlob* mtl_loadGlob = Signature(0xD541330);
	GfxRenderTarget* gfx_renderTargets = Signature(0xD573EB0);
	float& v_aspectRatioWindow = Signature(0xCC9D0E4);
	float& v_postProcessAspect = Signature(0xCC9D0FC);
	Material* floatz_display = Signature(0xFA5378);
	GfxCmdBufSourceState* gfx_cmdBufSourceState = Signature(0xD53F5F0);
	GfxCmdBufState* gfx_cmdBufState = Signature(0xD5404F0);
	GfxBackEndData* gfx_frontEndDataOut = Signature(0xCC9827C);
	GfxBackEndData* gfx_backEndData = Signature(0xD0704BC);
	GfxBuffers* gfx_buf = Signature(0xD2B0840);
	GfxWorld* gfx_world = Signature(0xD0701E0);
	GfxScene* scene = Signature(0xCF10280);
	GfxViewParms* viewParms = Signature(0xD540220);
	clipMap_t* cm = Signature(0x14098C0);
	ComWorld* com = Signature(0x1435CB8);
	materialCommands_t* tess = Signature(0xD085EE0);
	r_global_permanent_t* rgp = Signature(0xCC98280);

	// UI
	uintptr_t* ui_white_material = Signature(0xCAF06F0);
	UiContext* ui_cg_dc = Signature(0x746FA8);
	UiContext* ui_context = Signature(0xCAEE200);
	WinMouseVars_t* s_wmv = Signature(0xCC147C4);
	PlayerKeyState* player_keys = Signature(0x8F1DB8);
	clientUIActive_t* client_ui = Signature(0xC5F8F4);
	int* map_nameEnum = Signature(0xCAF2330);
	int* g_typeEnum = Signature(0xCAF1820);
	Fonts_t* g_fonts = Signature(0xCAF06F8);

	// GSC
	uintptr_t* gScrMemTreePub = Signature(0x14E8A04);
	uintptr_t* scrVarPub = Signature(0x15CA61C);
	int* scrVarPub_p4 = Signature(0x15CA620);
	char* error_message = Signature(0x1798378);
	char* error_type = Signature(0x1798777);
	int* scr_numParam = Signature(0x1794074);
	gentity_s* scr_g_entities = Signature(0x1288500);
	level_locals_t* level_locals = Signature(0x13EB6A8);

	// Console
	float* con_matchtxtColor_currentDvar = Signature(0x6BDF14);
	float* con_matchtxtColor_currentValue = Signature(0x6BDF24);
	float* con_matchtxtColor_defaultValue = Signature(0x6BDF34);
	float* con_matchtxtColor_dvarDescription = Signature(0x6BDF54);
	float* con_matchtxtColor_domainDescription = Signature(0x6BDF44);
	ConDrawInputGlob* con_drawInputGlob = Signature(0x8CC2C8);
	Con* con = Signature(0x8DC8C0);
	bool* con_isCatcherActive = Signature(0xC5F8F8);
	float* con_screenMinLeft = Signature(0x8ECB14);
	float* con_screenMinTop = Signature(0x8ECB18);
	float* con_screenMaxRight = Signature(0x8ECB1C);
	float* con_screenMaxBottom = Signature(0x8ECB20);
	bool* con_ignoreMatchPrefixOnly = Signature(0x736BB1);
	cmd_function_s* cmd_functions = Signature(0x1410B3C);
	field_t* g_consoleField = Signature(0x8F1B88);

	// Database
	HANDLE db_handle = Signature(0x14E89A4);
	uint16_t* db_hashTable = Signature(0xE62A80);
	XAssetHeader* db_xassetPool = Signature(0x7265E0);
	XZone* g_zones = Signature(0xFFEFD0);
	XAssetEntry* g_assetEntryPool = Signature(0xF0D640);
	DB_GetXAssetSizeHandler_t* DB_GetXAssetSizeHandlers = Signature(0x726A10);
	uint32_t* g_poolSize = Signature(0x7263A0);
	int* XAssetStdCount = nullptr;
	const char** zone_code_post_gfx_mp = Signature(0xCC9D128);
	const char** zone_localized_code_post_gfx_mp = Signature(0xCC9D134);
	const char** zone_ui_mp = Signature(0xCC9D12C);
	const char** zone_common_mp = Signature(0xCC9D130);
	const char** zone_localized_common_mp = Signature(0xCC9D138);
	const char** zone_mod = Signature(0xCC9D13C);

	// Filesystem
	const char* fs_game = Signature(0xCB19898);
	searchpath_s* fs_searchPaths = Signature(0xD5EC4DC);
}
