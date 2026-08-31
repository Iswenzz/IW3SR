#include "Functions.hpp"

// clang-format off
namespace IW3SR
{
	Function<int(WeaponDef* def, void* callback)>
		BG_AddWeapon = ASM_LOAD(BG_AddWeapon);

	Function<void(const trajectory_t* tr, int atTime, vec3& out)>
		BG_EvaluateTrajectory = ASM_LOAD(BG_EvaluateTrajectory);

	Function<int(const char* name)>
		BG_FindWeaponIndexForName = 0x416610;

	Function<WeaponDef*(const char* name)>
		BG_LoadWeaponDef = 0x41C310;

	Function<int(const char* name, void* callback)>
		BG_RegisterWeapon = 0x416660;

	Function<void(int itemIndex)>
		CG_RegisterItemVisuals = 0x454320;

	Function<void(trace_t* result, const vec3& start, const vec3& mins, const vec3& maxs,
		const vec3& end, int skipEntity, int tracemask)>
		CG_Trace = 0x45A230;

	Function<void(int localClientNum, const char* text)>
		Cbuf_AddText = ASM_LOAD(Cbuf_AddText);

	Function<void()>
		Com_ClientDObjClearAllSkel = 0x500F70;

	Function<void(int localClientNum)>
		CL_ClearState = ASM_LOAD(CL_ClearState);

	Function<void()>
		CL_DemoCompleted = 0x468DF0;

	Function<void(int localClientNum)>
		CL_DownloadsComplete = ASM_LOAD(CL_DownloadsComplete);

	Function<void(int localClientNum)>
		CL_InitDownloads = ASM_LOAD(CL_InitDownloads);

	Function<void()>
		CL_ReadDemoArchive = 0x468EA0;

	Function<void(int localClientNum)>
		CL_ReadDemoData = 0x468F90;

	Function<void(netadr_t from, msg_t* msg)>
		CL_ServersResponsePacket = ASM_LOAD(CL_ServersResponsePacket);

	Function<void(const char* mapName, const char* gametype)>
		CL_SetupForNewServerMap = ASM_LOAD(CL_SetupForNewServerMap);

	Function<void(int localClientNum, int controllerIndex, const char* text)>
		Cmd_ExecuteSingleCommand = 0x4F9AB0;

	Function<void(int code, const char* format, const char* message)>
		Com_Error = 0x4FD330;

	Function<char*(const char** pData, bool allowLineBreaks)>
		Com_ParseExt = 0x570FB0;

	Function<void(ConChannel channel, const char* msg, int error)>
		Com_PrintMessage = 0x4FCA50;

	Function<bool(const char* zoneName, DB_FILE_EXISTS_PATH path)>
		DB_FileExists = ASM_LOAD(DB_FileExists);

	Function<uint8_t(int type, const char* name)>
		DB_IsXAssetDefault = 0x4898A0;

	Function<void(const char *localName, const char *remoteName)>
		DL_BeginDownload = 0x500AE0;

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

	Function<void(const char* name, const char* value, int source)>
		Dvar_SetFromStringByNameFromSource = ASM_LOAD(Dvar_SetFromStringByNameFromSource);

	Function<void(const char* path, const char* dir)>
		FS_AddIwdFilesForGameDirectory = 0x55D8B0;

	Function<int(void* buffer, int length, int file)>
		FS_Read = 0x55C120;

	Function<int(const char* filename)>
		FS_SV_FOpenFileWrite = 0x502BF0;

	Function<void(gentity_s* ent)>
		G_FreeEntity = 0x4E3A50;

	Function<void(const vec3& end, int passEntityNum, trace_t* results, const vec3& start, int contentMask)>
		G_MissileTrace = ASM_LOAD(G_MissileTrace);

	Function<bool FASTCALL(GfxImage* image)>
		Image_SetDefaultTexture = 0x616DB0;

	Function<void(char* infoString, const char* key, const char* value)>
		Info_SetValueForKey = 0x572280;

	Function<bool(pmove_t* pm, pml_t* pml)>
		Jump_Check = ASM_LOAD(Jump_Check);

	Function<Material*(const char* material, int size)>
		Material_RegisterHandle = ASM_LOAD(Material_RegisterHandle);

	Function<int(const uint8_t* input, uint8_t* output, int readsize)>
		MSG_ReadBitsCompress = ASM_LOAD(MSG_ReadBitsCompress);

	Function<void(msg_t* msg, entityState_s* to, const entityState_s* from, int number)>
		MSG_ReadDeltaEntity = ASM_LOAD(MSG_ReadDeltaEntity);

	Function<char*(msg_t* msg)>
		MSG_ReadStringLine = ASM_LOAD(MSG_ReadStringLine);

	Function<bool(netsrc_t sock, int length, const void* data, netadr_t to)>
		NET_SendPacket = ASM_LOAD(NET_SendPacket);

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

	Function<void(pmove_t* pm, trace_t* results, const vec3& start, const vec3& mins, const vec3& maxs, 
		const vec3& end, int pass_entity_num, int content_mask)>
		PM_PlayerTrace = ASM_LOAD(PM_PlayerTrace);

	Function<void(const vec3& normal, const vec3& velIn, vec3& velOut)>
		PM_ProjectVelocity = ASM_LOAD(PM_ProjectVelocity);

	Function<void(pmove_t* pm, pml_t* pml, bool gravity)> 
		PM_StepSlideMove = 0x4155C0;

	Function<void(const char* text, int maxChars, Font_s* font, float x, float y,
		float xScale, float yScale, float rotation, int style, const vec4& color)>
		R_AddCmdDrawText = ASM_LOAD(R_AddCmdDrawText);

	Function<void(Material* material, float x, float y, float w, float h,
		float null1, float null2, float null3, float null4, const vec4& color)>
		R_AddCmdDrawStretchPic = ASM_LOAD(R_AddCmdDrawStretchPic);

	Function<void FASTCALL(const vec4& color, char* colorBytes)>
		R_ConvertColorToBytes = 0x493530;

	Function<Font_s*(const char* font, int size)>
		R_RegisterFont = 0x5F1EC0;

	Function<void(GfxCmdBufSourceState* source, float gameTime)>
		R_SetGameTime = ASM_LOAD(R_SetGameTime);

	Function<void()>
		R_BeginFrame = 0x5F75A0;

	Function<void()>
		R_EndFrame = 0x5F7680;

	Function<void()>
		R_SyncRenderThread = 0x5F78F0;

	Function<void(uint32_t drawType)>
		R_IssueRenderCommands = 0x5F6210;

	Function<void(uint32_t localClientNum)>
		R_ClearScene = ASM_LOAD(R_ClearScene);

	Function<void(const refdef_s* refdef)>
		R_SetLodOrigin = ASM_LOAD(R_SetLodOrigin);

	Function<void(const refdef_s* refdef)>
		R_RenderScene = ASM_LOAD(R_RenderScene);

	Function<int(const char* text, int maxChars, Font_s* font)>
		R_TextWidth = ASM_LOAD(R_TextWidth);

	Function<MaterialTechnique*(MaterialTechniqueType techType, Material* material)>
		RB_BeginSurface = ASM_LOAD(RB_BeginSurface);

	Function<void(int count, int width, GfxPointVertex* verts, bool depthTest)>
		RB_DrawLines3D = 0x613040;

	Function<void()>
		RB_EndTessSurface = 0x61A2F0;

	Function<void(unsigned int track, int fadeTime)>
		SND_StopBackground = ASM_LOAD(SND_StopBackground);

	Function<bool(int length, const void* data, netadr_t to)>
		Sys_SendPacket = ASM_LOAD(Sys_SendPacket);

	Function<void(int localClientNum)>
		UI_CloseAllMenus = 0x54B060;
}
// clang-format on
namespace IW3SR
{
	using namespace asmjit;

	ASM_FUNCTION(BG_AddWeapon)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 0x0C)); // callback
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // def
		a.call(0x4164F0);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(BG_EvaluateTrajectory)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::ecx, x86::dword_ptr(x86::ebp, 0x10)); // out
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x0C)); // atTime
		a.push(x86::dword_ptr(x86::ebp, 0x08));			 // tr
		a.call(0x40BD70);
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(DB_FileExists)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // path
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // zoneName
		a.call(0x48B9B0);
		a.add(x86::esp, 0x04);
		a.movzx(x86::eax, x86::al);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Dvar_FindVar)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 0x08)); // name
		a.call(0x56B5D0);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);

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
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);
		a.add(x86::esp, 0x24);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Dvar_SetFromStringByNameFromSource)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // source
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // value
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // name
		a.call(0x56D0A0);
		a.add(x86::esp, 0x08);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(G_MissileTrace)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x18));			 // contentMask
		a.push(x86::dword_ptr(x86::ebp, 0x14));			 // start
		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // results
		a.mov(x86::edx, x86::dword_ptr(x86::ebp, 0x0C)); // passEntityNum
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // end
		a.call(0x4C4FD0);
		a.add(x86::esp, 0x0C);

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
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Material_RegisterHandle)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edx, x86::dword_ptr(x86::ebp, 0x0C)); // size
		a.mov(x86::ecx, x86::dword_ptr(x86::ebp, 0x08)); // material
		a.call(0x5F2AA0);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(MSG_ReadBitsCompress)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);

		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // readsize
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // output
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // input
		a.mov(x86::ecx, 0x5053C0);						 // Scratch, the callee reads eax and the stack only
		a.call(x86::ecx);

		a.mov(x86::esp, x86::ebp);
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(MSG_ReadDeltaEntity)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);

		a.push(x86::dword_ptr(x86::ebp, 0x14));			 // number
		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // from
		a.push(imm(0));									 // time
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x0C)); // to
		a.mov(x86::ecx, x86::dword_ptr(x86::ebp, 0x08)); // msg
		a.mov(x86::edx, 0x506E20);
		a.call(x86::edx);

		a.mov(x86::esp, x86::ebp);
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(MSG_ReadStringLine)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edx, x86::dword_ptr(x86::ebp, 0x08)); // msg
		a.call(0x505890);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(Cbuf_AddText)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);

		a.mov(x86::ecx, x86::dword_ptr(x86::ebp, 0x08)); // localClientNum
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x0C)); // text
		a.mov(x86::edx, 0x4F8D90);
		a.call(x86::edx);

		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(CL_SetupForNewServerMap)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.push(x86::edi);

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // gametype
		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 0x08)); // mapName
		a.mov(x86::eax, 0x470580);
		a.call(x86::eax);
		a.add(x86::esp, 0x04);

		a.pop(x86::edi);
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(CL_ClearState)
	{
		a.push(x86::esi);
		a.mov(x86::esi, x86::dword_ptr(x86::esp, 0x08)); // localClientNum
		a.mov(x86::eax, 0x461DA0);
		a.call(x86::eax);
		a.pop(x86::esi);
		a.ret();
	}

	ASM_FUNCTION(CL_DownloadsComplete)
	{
		a.push(x86::edi);
		a.mov(x86::edi, x86::dword_ptr(x86::esp, 0x08)); // localClientNum
		a.mov(x86::eax, 0x46A8D0);
		a.call(x86::eax);
		a.pop(x86::edi);
		a.ret();
	}

	ASM_FUNCTION(CL_InitDownloads)
	{
		a.mov(x86::eax, x86::dword_ptr(x86::esp, 0x04)); // localClientNum
		a.mov(x86::ecx, 0x46AC60);
		a.call(x86::ecx);
		a.ret();
	}

	ASM_FUNCTION(CL_ServersResponsePacket)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x18)); // from, copied by value
		a.push(x86::dword_ptr(x86::ebp, 0x14));
		a.push(x86::dword_ptr(x86::ebp, 0x10));
		a.push(x86::dword_ptr(x86::ebp, 0x0C));
		a.push(x86::dword_ptr(x86::ebp, 0x08));
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x1C)); // msg
		a.call(0x4714B0);
		a.add(x86::esp, 0x14);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(NET_SendPacket)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x24)); // to, copied by value
		a.push(x86::dword_ptr(x86::ebp, 0x20));
		a.push(x86::dword_ptr(x86::ebp, 0x1C));
		a.push(x86::dword_ptr(x86::ebp, 0x18));
		a.push(x86::dword_ptr(x86::ebp, 0x14));
		a.push(x86::dword_ptr(x86::ebp, 0x08));			 // sock
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x0C)); // length
		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 0x10)); // data
		a.call(0x508B40);
		a.movzx(x86::eax, x86::al);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);
		a.add(x86::esp, 0x18);

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
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);
		a.add(x86::esp, 0x08);

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
		a.add(x86::esp, 0x04);

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
		a.add(x86::esp, 0x04);

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
		a.add(x86::esp, 0x04);

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
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(RB_BeginSurface)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::edi, x86::dword_ptr(x86::ebp, 0x08)); // techType
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x0C)); // material
		a.call(0x61A220);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);

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

		a.push(x86::dword_ptr(x86::ebp, 0x2C));			 // color
		a.push(x86::dword_ptr(x86::ebp, 0x28));			 // null4
		a.push(x86::dword_ptr(x86::ebp, 0x24));			 // null3
		a.push(x86::dword_ptr(x86::ebp, 0x20));			 // null2
		a.push(x86::dword_ptr(x86::ebp, 0x1C));			 // null1
		a.push(x86::dword_ptr(x86::ebp, 0x18));			 // h
		a.push(x86::dword_ptr(x86::ebp, 0x14));			 // w
		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // y
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // x
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // material
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

		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // gameTime
		a.mov(x86::esi, x86::dword_ptr(x86::ebp, 0x08)); // source
		a.call(0x6490E0);
		a.add(x86::esp, 0x04);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_ClearScene)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // localClientNum
		a.call(0x5FA840);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_SetLodOrigin)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // refdef
		a.call(0x5FAE90);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_RenderScene)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // refdef
		a.call(0x5FAF00);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(R_TextWidth)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x10));			 // font
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // maxChars
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // text
		a.call(0x5F1EE0);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);
		a.add(x86::esp, 0x08);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}

	ASM_FUNCTION(SND_StopBackground)
	{
		a.mov(x86::ecx, 0x5C4770);						 // Scratch, the callee reads edx and the stack only
		a.mov(x86::edx, x86::dword_ptr(x86::esp, 0x04)); // track
		a.mov(x86::eax, x86::dword_ptr(x86::esp, 0x08)); // fadeTime
		a.push(x86::eax);
		a.call(x86::ecx);
		a.add(x86::esp, 0x04);
		a.ret();
	}

	ASM_FUNCTION(Sys_SendPacket)
	{
		a.push(x86::ebp);
		a.mov(x86::ebp, x86::esp);
		a.pushad();

		a.push(x86::dword_ptr(x86::ebp, 0x20)); // to, copied by value
		a.push(x86::dword_ptr(x86::ebp, 0x1C));
		a.push(x86::dword_ptr(x86::ebp, 0x18));
		a.push(x86::dword_ptr(x86::ebp, 0x14));
		a.push(x86::dword_ptr(x86::ebp, 0x10));
		a.push(x86::dword_ptr(x86::ebp, 0x0C));			 // data
		a.mov(x86::eax, x86::dword_ptr(x86::ebp, 0x08)); // length
		a.call(0x577F50);
		a.movzx(x86::eax, x86::al);
		a.mov(x86::dword_ptr(x86::ebp, -0x04), x86::eax);
		a.add(x86::esp, 0x18);

		a.popad();
		a.pop(x86::ebp);
		a.ret();
	}
}
