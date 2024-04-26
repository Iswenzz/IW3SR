#pragma once
#include "Structs.hpp"

#include "Core/Memory/Assembler.hpp"
#include "Core/Memory/Function.hpp"

// clang-format off
namespace IW3SR
{
	API extern Function<void(const trajectory_t* tr, int atTime, float* out)>
		BG_EvaluateTrajectory;

	API extern Function<int(const char* name)>
		BG_FindWeaponIndexByName;

	API extern Function<void(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int skipEntity, int tracemask)>
		CG_Trace;

	extern Function<void(int localClientNum, int controllerIndex, const char* text)>
		Cmd_ExecuteSingleCommand;

	extern Function<char*(const char** pData, bool allowLineBreaks)>
		Com_ParseExt;

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

	extern Function<Material*(const char* material, int size)>
		Material_RegisterHandle;

	API extern Function<void(pmove_t* pm, int entity_num)>
		PM_AddTouchEnt;

	API extern Function<bool(pmove_t *pm, pml_t *pml, trace_t *trace)>
		PM_CorrectAllSolid;

	API extern Function<void(playerState_s* ps, pml_t* pml)>
		PM_CrashLand;

	API extern Function<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTraceMissed;

	API extern Function<void(pmove_t* pm, trace_t* results, const float* start, const float* mins, const float* maxs, 
		const float* end, int pass_entity_num, int content_mask)>
		PM_PlayerTrace;

	API extern Function<void(float* normal, float* velIn, float* velOut)>
		PM_ProjectVelocity;

	API extern Function<void(pmove_t* pm, pml_t* pml, bool gravity)> 
		PM_StepSlideMove;

	extern Function<void(const char* text, int maxChars, Font_s* font, float x, float y,
		float xScale, float yScale, float rotation, int style, float* color)>
		R_AddCmdDrawText;

	extern Function<void(Material* material, float x, float y, float w, float h,
		float null1, float null2, float null3, float null4, float* color)>
		R_AddCmdDrawStretchPic;

	extern Function<void FASTCALL(const float* colorFloat, char* colorBytes)>
		R_ConvertColorToBytes;

	extern Function<Font_s*(const char* font, int size)>
		R_RegisterFont;

	extern Function<void(GfxCmdBufSourceState* source, float gameTime)>
		R_SetGameTime;

	extern Function<int(const char* text, int maxChars, Font_s* font)>
		R_TextWidth;

	extern Function<void(int count, int width, GfxPointVertex* verts, bool depthTest)>
		RB_DrawLines3D;
}
// clang-format on
namespace IW3SR
{
	ASM_FUNCTION(BG_EvaluateTrajectory);
	ASM_FUNCTION(Dvar_FindVar);
	ASM_FUNCTION(Dvar_RegisterVariant);
	ASM_FUNCTION(PM_AddTouchEnt);
	ASM_FUNCTION(PM_CorrectAllSolid);
	ASM_FUNCTION(PM_CrashLand);
	ASM_FUNCTION(PM_GroundTraceMissed);
	ASM_FUNCTION(PM_PlayerTrace);
	ASM_FUNCTION(PM_ProjectVelocity);
	ASM_FUNCTION(R_AddCmdDrawText);
	ASM_FUNCTION(R_AddCmdDrawStretchPic);
	ASM_FUNCTION(R_SetGameTime);
	ASM_FUNCTION(R_TextWidth);
}
