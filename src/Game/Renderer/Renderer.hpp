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
		static void Commands(void* cmds);
		static void STDCALL Frame(IDirect3DDevice9* device);
		static HRESULT STDCALL Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);
	};
}
