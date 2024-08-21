#include "Functions.hpp"

// clang-format off
namespace IW3SR
{
	Function<void(const trajectory_t* tr, int atTime, float* out)>
		BG_EvaluateTrajectory = ASM_LOAD(BG_EvaluateTrajectory);

	Function<int(const char* name)>
		BG_FindWeaponIndexByName = 0x416610;

	Function<void(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int skipEntity, int tracemask)>
		CG_Trace = 0x45A230;

	Function<void(int localClientNum, int controllerIndex, const char* text)>
		Cmd_ExecuteSingleCommand = 0x4F9AB0;

	Function<char*(const char** pData, bool allowLineBreaks)>
		Com_ParseExt = 0x570FB0;

	Function<dvar_s*(const char* name)>
		Dvar_FindVar = ASM_LOAD(Dvar_FindVar);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		int value, int null1, int null2, int null3, int min, int max)>
		Dvar_RegisterVariantInt = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float value, int null1, int null2, int null3, float min, float max)>
		Dvar_RegisterVariantFloat = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		bool value, int null1, int null2, int null3, int null4, int null5)>
		Dvar_RegisterVariantBool = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		const char* value, int null1, int null2, int null3, int null4, int null5)>
		Dvar_RegisterVariantString = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		int value, int null1, int null2, int null3, int enumSize, const char** enumData)>
		Dvar_RegisterVariantEnum = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float x, float y, int null2, int null3, float min, float max)>
		Dvar_RegisterVariantVec2 = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float x, float y, int z, int null3, float min, float max)>
		Dvar_RegisterVariantVec3 = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float x, float y, int z, int w, float min, float max)>
		Dvar_RegisterVariantVec4 = ASM_LOAD(Dvar_RegisterVariant);

	Function<dvar_s*(const char* dvarName, DvarType type, int flags, const char* description,
		float r, float g, int b, int a, int null4, int null5)>
		Dvar_RegisterVariantColor = ASM_LOAD(Dvar_RegisterVariant);

	Function<void(float* end, int passEntityNum, trace_t* results, float* start, int contentMask)>
		G_MissileTrace = ASM_LOAD(G_MissileTrace);

	Function<bool(pmove_t* pm, pml_t* pml)>
		Jump_Check = ASM_LOAD(Jump_Check);

	Function<Material*(const char* material, int size)>
		Material_RegisterHandle = 0x5F2A80;

	Function<void(pmove_t* pm)>
		PmoveSingle = 0x4143A0;

	Function<void(pmove_t* pm, int entity_num)>
		PM_AddTouchEnt = ASM_LOAD(PM_AddTouchEnt);

	Function<bool(pmove_t *pm, pml_t *pml, trace_t *trace)>
		PM_CorrectAllSolid = ASM_LOAD(PM_CorrectAllSolid);

	Function<void(playerState_s* ps, pml_t* pml)>
		PM_CrashLand = ASM_LOAD(PM_CrashLand);

	Function<void(playerState_s* ps, pml_t* pml)>
		PM_Friction = ASM_LOAD(PM_Friction);

	Function<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTrace = 0x410660;

	Function<void(pmove_t* pm, pml_t* pml)>
		PM_GroundTraceMissed = ASM_LOAD(PM_GroundTraceMissed);

	Function<void(pmove_t* pm, trace_t* results, const float* start, const float* mins, const float* maxs, 
		const float* end, int pass_entity_num, int content_mask)>
		PM_PlayerTrace = ASM_LOAD(PM_PlayerTrace);

	Function<void(float* normal, float* velIn, float* velOut)>
		PM_ProjectVelocity = ASM_LOAD(PM_ProjectVelocity);

	Function<void(pmove_t* pm, pml_t* pml, bool gravity)> 
		PM_StepSlideMove = 0x4155C0;

	Function<void(const char* text, int maxChars, Font_s* font, float x, float y,
		float xScale, float yScale, float rotation, int style, float* color)>
		R_AddCmdDrawText = ASM_LOAD(R_AddCmdDrawText);

	Function<void(Material* material, float x, float y, float w, float h,
		float null1, float null2, float null3, float null4, float* color)>
		R_AddCmdDrawStretchPic = ASM_LOAD(R_AddCmdDrawStretchPic);

	Function<void FASTCALL(const float* colorFloat, char* colorBytes)>
		R_ConvertColorToBytes = 0x493530;

	Function<Font_s*(const char* font, int size)>
		R_RegisterFont = 0x5F1EC0;

	Function<void(GfxCmdBufSourceState* source, float gameTime)>
		R_SetGameTime = ASM_LOAD(R_SetGameTime);

	Function<int(const char* text, int maxChars, Font_s* font)>
		R_TextWidth = ASM_LOAD(R_TextWidth);

	Function<void(int count, int width, GfxPointVertex* verts, bool depthTest)>
		RB_DrawLines3D = 0x613040;

	Function<void(DWORD* serverPacket, DWORD packet)>
		ScreenshotRequest = COD4X_BASE + 0xEA610;
}
// clang-format on
namespace IW3SR
{
	using namespace asmjit;

	ASM_FUNCTION(BG_EvaluateTrajectory)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::ecx, x86::dword_ptr(x86::ebp, 0x10)); // out
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x0C)); // atTime
		a.push(x86::dword_ptr(x86::ebp, 8));			 // tr
		a.call(0x40BD70);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Dvar_FindVar)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 8)); // name
		a.call(0x56B5D0);
		a.mov(x86::dword_ptr(x86::esp, 0x1C), x86::eax);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Dvar_RegisterVariant)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x2C));			 // max
		a.push(x86::dword_ptr(x86::ebp, 0x28));			 // min
		a.push(x86::dword_ptr(x86::ebp, 0x24));			 // w
		a.push(x86::dword_ptr(x86::ebp, 0x20));			 // z
		a.push(x86::dword_ptr(x86::ebp, 0x1C));			 // y
		a.push(x86::dword_ptr(x86::ebp, 0x18));			 // x
		a.push(x86::dword_ptr(x86::ebp, 0x14));			 // description
		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // flags
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // type
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // dvarName
		a.call(0x56C350);
		a.mov(x86::dword_ptr(x86::esp, 0x40), x86::eax);
		a.add(x86::esp, 0x24);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(G_MissileTrace)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x18)); // contentMask
		a.push(x86::dword_ptr(x86::ebp, 0x14)); // start
		a.push(x86::dword_ptr(x86::ebp, 0x10)); // results
		a.push(x86::dword_ptr(x86::ebp, 0x0C)); // passEntityNum
		a.push(x86::dword_ptr(x86::ebp, 0x08)); // end
		a.call(0x4C4FD0);
		a.add(x86::esp, 0x14);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Jump_Check)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // pml
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // pm
		a.call(0x407D90);
		a.mov(x86::dword_ptr(x86::esp, 0x20), x86::eax);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_AddTouchEnt)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.movzx(x86::edi, x86::dword_ptr(x86::ebp, 0x0C)); // entity_num
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08));   // pm
		a.call(0x40E270);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_CorrectAllSolid)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // trace
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // pml
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // pm
		a.call(0x410370);
		a.mov(x86::dword_ptr(x86::esp, 0x24), x86::eax);
		a.add(x86::esp, 8);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_CrashLand)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // pml
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x08)); // ps
		a.call(0x40FFB0);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_Friction)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // pml
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x08)); // ps
		a.call(0x40E860);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_GroundTraceMissed)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // pml
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // pm
		a.call(0x4104E0);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_PlayerTrace)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x24));			 // content_mask
		a.push(x86::dword_ptr(x86::ebp, 0x20));			 // pass_entity_num
		a.push(x86::dword_ptr(x86::ebp, 0x1C));			 // end
		a.push(x86::dword_ptr(x86::ebp, 0x18));			 // maxs
		a.push(x86::dword_ptr(x86::ebp, 0x14));			 // mins
		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // start
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // results
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x08)); // pm
		a.call(0x40E160);
		a.add(x86::esp, 0x1C);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(PM_ProjectVelocity)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 0x10)); // normal
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x0C)); // velIn
		a.push(x86::dword_ptr(x86::ebp, 0x08));			 // velOut
		a.call(0x40E330);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_AddCmdDrawText)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::ecx, x86::dword_ptr(x86::ebp, 0x2C)); // color
		a.push(x86::dword_ptr(x86::ebp, 0x28));			 // style
		a.push(x86::dword_ptr(x86::ebp, 0x24));			 // rotation
		a.push(x86::dword_ptr(x86::ebp, 0x20));			 // yScale
		a.push(x86::dword_ptr(x86::ebp, 0x1C));			 // xScale
		a.push(x86::dword_ptr(x86::ebp, 0x18));			 // y
		a.push(x86::dword_ptr(x86::ebp, 0x14));			 // x
		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // font
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // maxChars
		a.push(x86::dword_ptr(x86::ebp, 0x08));			 // text
		a.call(0x5F6B00);
		a.add(x86::esp, 0x24);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_AddCmdDrawStretchPic)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x2C));		  // color
		a.push(x86::dword_ptr(x86::ebp, 0x28));		  // null4
		a.push(x86::dword_ptr(x86::ebp, 0x24));		  // null3
		a.push(x86::dword_ptr(x86::ebp, 0x20));		  // null2
		a.push(x86::dword_ptr(x86::ebp, 0x1C));		  // null1
		a.push(x86::dword_ptr(x86::ebp, 0x18));		  // h
		a.push(x86::dword_ptr(x86::ebp, 0x14));		  // w
		a.push(x86::dword_ptr(x86::ebp, 0x10));		  // y
		a.push(x86::dword_ptr(x86::ebp, 0x0C));		  // x
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 8)); // material
		a.call(0x5F65F0);
		a.add(x86::esp, 0x24);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_SetGameTime)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x0C));		  // gameTime
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 8)); // source
		a.call(0x6490E0);
		a.add(x86::esp, 4);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_TextWidth)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x10));		  // font
		a.push(x86::dword_ptr(x86::ebp, 0x0C));		  // maxChars
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 8)); // text
		a.call(0x5F1EE0);
		a.mov(x86::dword_ptr(x86::esp, 0x24), x86::eax);
		a.add(x86::esp, 8);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}
}
