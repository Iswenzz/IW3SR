#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class GRenderer
	{
	public:
		static void Initialize();
		static void Shutdown(int window);

		static void Draw2D(int localClientNum);
		static void Draw3D(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf);
		static void ExecuteRenderCommandsLoop(void* cmds);
		static void STDCALL Frame(IDirect3DDevice9* device);
		static HRESULT STDCALL Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);
		static void UpdateMaterials();

		static void AddCmdDrawText(const char** text, int maxChars, Font_s* font, float x, float y, float xScale,
			float yScale, float rotation, int style, const vec4& color);
		static void AddCmdDrawTextWithEffects(const char* text, int maxChars, Font_s* font, float x, float y,
			float xScale, float yScale, float rotation, const vec4& color, int style, const vec4& glowColor,
			Material* fxMaterial, Material* fxMaterialGlow, int fxBirthTime, int fxLetterTime, int fxDecayStartTime,
			int fxDecayDuration);

	private:
		static bool IsRedCubemap(IDirect3DCubeTexture9* cubemap);
	};
}
