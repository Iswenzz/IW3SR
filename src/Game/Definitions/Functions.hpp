#pragma once
#include "Structs.hpp"

#include "Engine/Core/Memory/Assembler.hpp"
#include "Engine/Core/Memory/Function.hpp"

// clang-format off
namespace IW3SR
{
	API extern Function<int(WeaponDef* def, void* callback)>
		BG_AddWeapon;

	API extern Function<void(const trajectory_t* tr, int atTime, vec3& out)>
		BG_EvaluateTrajectory;

	API extern Function<int(const char* name)>
		BG_FindWeaponIndexForName;

	API extern Function<WeaponDef*(const char* name)>
		BG_LoadWeaponDef;

	API extern Function<int(const char* name, void* callback)>
		BG_RegisterWeapon;

	API extern Function<void(int itemIndex)>
		CG_RegisterItemVisuals;

	API extern Function<void(trace_t* result, const vec3& start, const vec3& mins, const vec3& maxs,
		const vec3& end, int skipEntity, int tracemask)>
		CG_Trace;

	extern Function<void(int localClientNum, const char* text)>
		Cbuf_AddText;

	extern Function<void()>
		Com_ClientDObjClearAllSkel;

	extern Function<void(int localClientNum)>
		CL_ClearState;

	extern Function<void()>
		CL_DemoCompleted;

	extern Function<void(int localClientNum)>
		CL_DownloadsComplete;

	extern Function<void(int localClientNum)>
		CL_InitDownloads;

	extern Function<void()>
		CL_ReadDemoArchive;

	extern Function<void(int localClientNum)>
		CL_ReadDemoData;

	extern Function<void(netadr_t from, msg_t* msg)>
		CL_ServersResponsePacket;

	extern Function<void(const char* mapName, const char* gametype)>
		CL_SetupForNewServerMap;

	extern Function<void(int localClientNum, int controllerIndex, const char* text)>
		Cmd_ExecuteSingleCommand;

	API extern Function<void(int code, const char* format, const char* message)>
		Com_Error;

	extern Function<char*(const char** pData, bool allowLineBreaks)>
		Com_ParseExt;

	API extern Function<void(ConChannel channel, const char* msg, int error)>
		Com_PrintMessage;

	API extern Function<bool(const char* zoneName, DB_FILE_EXISTS_PATH path)>
		DB_FileExists;

	extern Function<uint8_t(int type, const char* name)>
		DB_IsXAssetDefault;

	extern Function<void(const char *localName, const char *remoteName)>
		DL_BeginDownload;

	extern Function<dvar_s*(const char* name)>
		Dvar_FindVar;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		int value, int null1, int null2, int null3, int min, int max)>
		Dvar_RegisterVariantInt;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float value, int null1, int null2, int null3, float min, float max)>
		Dvar_RegisterVariantFloat;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		bool value, int null1, int null2, int null3, int null4, int null5)>
		Dvar_RegisterVariantBool;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		const char* value, int null1, int null2, int null3, int null4, int null5)>
		Dvar_RegisterVariantString;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		int value, int null1, int null2, int null3, int enumSize, const char** enumData)>
		Dvar_RegisterVariantEnum;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float x, float y, int null2, int null3, float min, float max)>
		Dvar_RegisterVariantVec2;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float x, float y, int z, int null3, float min, float max)>
		Dvar_RegisterVariantVec3;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float x, float y, int z, int w, float min, float max)>
		Dvar_RegisterVariantVec4;

	extern Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float r, float g, int b, int a, int null4, int null5)>
		Dvar_RegisterVariantColor;

	extern Function<void(const char* name, const char* value, int source)>
		Dvar_SetFromStringByNameFromSource;

	extern Function<void(const char* path, const char* dir)>
		FS_AddIwdFilesForGameDirectory;

	extern Function<int(void* buffer, int length, int file)>
		FS_Read;

	extern Function<int(const char* filename)>
		FS_SV_FOpenFileWrite;

	API extern Function<void(gentity_s* ent)>
		G_FreeEntity;

	API extern Function<void(const vec3& end, int passEntityNum, trace_t* results, const vec3& start, int contentMask)>
		G_MissileTrace;

	extern Function<bool FASTCALL(GfxImage* image)>
		Image_SetDefaultTexture;

	extern Function<void(char* infoString, const char* key, const char* value)>
		Info_SetValueForKey;

	API extern Function<bool(pmove_t* pm, pml_t* pml)>
		Jump_Check;

	API extern Function<Material*(const char* material, int size)>
		Material_RegisterHandle;

	extern Function<int(const uint8_t* input, uint8_t* output, int readsize)>
		MSG_ReadBitsCompress;

	extern Function<void(msg_t* msg, entityState_s* to, const entityState_s* from, int number)>
		MSG_ReadDeltaEntity;

	extern Function<char*(msg_t* msg)>
		MSG_ReadStringLine;

	extern Function<bool(netsrc_t sock, int length, const void* data, netadr_t to)>
		NET_SendPacket;

	API extern Function<void(pmove_t* pm)>
		PmoveSingle;

	API extern Function<void(pmove_t* pm, int entity_num)>
		PM_AddTouchEnt;

	API extern Function<bool(pmove_t *pm, pml_t *pml, trace_t *trace)>
		PM_CorrectAllSolid;

	API extern Function<void(playerState_s* ps, pml_t* pml)>
		PM_CrashLand;

	API extern Function<void(playerState_s* ps, pml_t* pml)>
		PM_Friction;

	API extern Function<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace;

	API extern Function<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTraceMissed;

	API extern Function<void(pmove_t* pm, trace_t* results, const vec3& start, const vec3& mins, const vec3& maxs,
		const vec3& end, int pass_entity_num, int content_mask)>
		PM_PlayerTrace;

	API extern Function<void(const vec3& normal, const vec3& velIn, vec3& velOut)>
		PM_ProjectVelocity;

	API extern Function<void(pmove_t* pm, pml_t* pml, bool gravity)>
		PM_StepSlideMove;

	extern Function<void(const char* text, int maxChars, Font_s* font, float x, float y,
		float xScale, float yScale, float rotation, int style, const vec4& color)>
		R_AddCmdDrawText;

	API extern Function<void(Material* material, float x, float y, float w, float h,
		float null1, float null2, float null3, float null4, const vec4& color)>
		R_AddCmdDrawStretchPic;

	extern Function<void FASTCALL(const vec4& color, char* colorBytes)>
		R_ConvertColorToBytes;

	extern Function<Font_s*(const char* font, int size)>
		R_RegisterFont;

	extern Function<void(GfxCmdBufSourceState* source, float gameTime)>
		R_SetGameTime;

	API extern Function<void()>
		R_BeginFrame;

	API extern Function<void()>
		R_EndFrame;

	API extern Function<void()>
		R_SyncRenderThread;

	API extern Function<void(uint32_t drawType)>
		R_IssueRenderCommands;

	API extern Function<void(uint32_t localClientNum)>
		R_ClearScene;

	API extern Function<void(const refdef_s* refdef)>
		R_SetLodOrigin;

	API extern Function<void(const refdef_s* refdef)>
		R_RenderScene;

	extern Function<int(const char* text, int maxChars, Font_s* font)>
		R_TextWidth;

	API extern Function<MaterialTechnique*(MaterialTechniqueType techType, Material* material)>
		RB_BeginSurface;

	extern Function<void(int count, int width, GfxPointVertex* verts, bool depthTest)>
		RB_DrawLines3D;

	API extern Function<void()>
		RB_EndTessSurface;

	extern Function<void(unsigned int track, int fadeTime)>
		SND_StopBackground;

	extern Function<bool(int length, const void* data, netadr_t to)>
		Sys_SendPacket;

	extern Function<void(int localClientNum)>
		UI_CloseAllMenus;
}
// clang-format on
namespace IW3SR
{
	ASM_FUNCTION(BG_AddWeapon);
	ASM_FUNCTION(BG_EvaluateTrajectory);
	ASM_FUNCTION(Cbuf_AddText);
	ASM_FUNCTION(CL_ClearState);
	ASM_FUNCTION(CL_DownloadsComplete);
	ASM_FUNCTION(CL_InitDownloads);
	ASM_FUNCTION(DB_FileExists);
	ASM_FUNCTION(Dvar_FindVar);
	ASM_FUNCTION(Dvar_RegisterVariant);
	ASM_FUNCTION(Dvar_SetFromStringByNameFromSource);
	ASM_FUNCTION(G_MissileTrace);
	ASM_FUNCTION(Jump_Check);
	ASM_FUNCTION(Material_RegisterHandle);
	ASM_FUNCTION(MSG_ReadBitsCompress);
	ASM_FUNCTION(MSG_ReadDeltaEntity);
	ASM_FUNCTION(MSG_ReadStringLine);
	ASM_FUNCTION(CL_ServersResponsePacket);
	ASM_FUNCTION(CL_SetupForNewServerMap);
	ASM_FUNCTION(NET_SendPacket);
	ASM_FUNCTION(PM_AddTouchEnt);
	ASM_FUNCTION(PM_CorrectAllSolid);
	ASM_FUNCTION(PM_CrashLand);
	ASM_FUNCTION(PM_Friction);
	ASM_FUNCTION(PM_GroundTraceMissed);
	ASM_FUNCTION(PM_PlayerTrace);
	ASM_FUNCTION(PM_ProjectVelocity);
	ASM_FUNCTION(RB_BeginSurface);
	ASM_FUNCTION(R_AddCmdDrawText);
	ASM_FUNCTION(R_AddCmdDrawStretchPic);
	ASM_FUNCTION(R_SetGameTime);
	ASM_FUNCTION(R_ClearScene);
	ASM_FUNCTION(R_SetLodOrigin);
	ASM_FUNCTION(R_RenderScene);
	ASM_FUNCTION(R_TextWidth);
	ASM_FUNCTION(SND_StopBackground);
	ASM_FUNCTION(Sys_SendPacket);
}
