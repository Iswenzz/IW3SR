#pragma once
#include "Structs.hpp"

namespace IW3SR
{
	// Binaries
	API extern std::string COD4X_BIN;
	API extern std::string IW3MP_BIN;
	API extern std::string IW3XO_BIN;

	// Base
	API extern uintptr_t COD4X_BASE;
	API extern uintptr_t IW3MP_BASE;
	API extern uintptr_t IW3XO_BASE;

	// Game
	API extern CmdArgs* cmd_args;
	API extern cmd_function_s** cmds;
	API extern clientActive_t* clients;
	API extern clientStatic_t* cls;
	API extern clientConnection_t& clc;
	API extern cg_s* cgs;
	API extern centity_s* cg_entities;
	extern int& com_frameTime;
	extern float& com_timescaleValue;
	API extern int* g_entities;
	API extern int* g_clients;
	API extern pmove_t* pmove;
	extern const char* g_entityBeginParsePoint;
	extern const char* g_entityEndParsePoint;
	API extern int* clientActiveCmdNumber;
	extern infoParm_t* info_params;
	extern WeaponDef** bg_weaponNames;
	extern dvar_s** dvars;
	extern int& dvarCount;

	// Render
	API extern DxGlobals* dx;
	extern MaterialLoadGlob* mtl_loadGlob;
	extern GfxRenderTarget* gfx_renderTargets;
	extern float& v_aspectRatioWindow;
	extern float& v_postProcessAspect;
	extern Material* floatz_display;
	extern GfxCmdBufSourceState* gfx_cmdBufSourceState;
	extern GfxCmdBufState* gfx_cmdBufState;
	extern GfxBackEndData* gfx_frontEndDataOut;
	extern GfxBackEndData* gfx_backEndData;
	extern GfxBuffers* gfx_buf;
	extern GfxWorld* gfx_world;
	extern GfxScene* scene;
	extern GfxViewParms* viewParms;
	extern clipMap_t* cm;
	extern ComWorld* com;
	extern materialCommands_t* tess;
	extern r_global_permanent_t* rgp;

	// UI
	extern uintptr_t* ui_white_material;
	extern int* g_typeEnum;
	extern int* map_nameEnum;
	extern UiContext* ui_cg_dc;
	extern UiContext* ui_context;
	extern WinMouseVars_t* s_wmv;
	extern PlayerKeyState* player_keys;
	extern clientUIActive_t* client_ui;
	extern Fonts_t* g_fonts;

	// GSC
	API extern uintptr_t* gScrMemTreePub;
	API extern uintptr_t* scrVarPub;
	extern int* scrVarPub_p4;
	extern char* errorMessage;
	extern char* errorType;
	extern int* scr_numParam;
	API extern gentity_s* scr_g_entities;
	API extern level_locals_t* level_locals;

	// Console
	extern float* con_matchtxtColor_currentDvar;
	extern float* con_matchtxtColor_currentValue;
	extern float* con_matchtxtColor_defaultValue;
	extern float* con_matchtxtColor_dvarDescription;
	extern float* con_matchtxtColor_domainDescription;
	extern ConDrawInputGlob* con_drawInputGlob;
	extern Con* con;
	extern field_t* g_consoleField;
	extern bool* key_isCatcherActive;
	extern float* con_screenMinLeft;
	extern float* con_screenMinTop;
	extern float* con_screenMaxRight;
	extern float* con_screenMaxBottom;
	extern bool* con_ignoreMatchPrefixOnly;
	extern cmd_function_s* cmd_functions;

	// Database
	extern HANDLE db_handle;
	extern uint16_t* db_hashTable;
	extern XAssetHeader* db_xassetPool;
	extern XZone* g_zones;
	extern XAssetEntry* g_assetEntryPool;
	extern DB_XAssetSizeHandler_t* db_xassetSizeHandlers;
	extern uint32_t* g_poolSize;
	extern const char** zone_code_post_gfx_mp;
	extern const char** zone_localized_code_post_gfx_mp;
	extern const char** zone_ui_mp;
	extern const char** zone_common_mp;
	extern const char** zone_localized_common_mp;
	extern const char** zone_mod;

	// Filesystem
	extern const char* fs_game;
	extern searchpath_s* fs_searchPaths;
}
