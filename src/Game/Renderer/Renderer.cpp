#include "Renderer.hpp"

#include "Drawing/Draw2D.hpp"
#include "Drawing/Draw3D.hpp"
#include "Modules/Modules.hpp"
#include "Modules/Settings.hpp"
#include "UI/UI.hpp"

namespace IW3SR
{
	void GRenderer::Initialize()
	{
		R_Init_h();

		IDirect3DDevice9_Reset_h.Update(VTABLE(dx->device, 16));
		IDirect3DDevice9_EndScene_h.Update(VTABLE(dx->device, 42));

		Dvar::Initialize();
		Settings::Deserialize();
		Modules::Deserialize();

		Device::Swap(dx->d3d9, dx->device);
		Renderer::Initialize();
		GUI::Initialize();
	}

	void GRenderer::Shutdown(int window)
	{
		Renderer::Shutdown();

		Modules::Serialize();
		Settings::Serialize();

		R_Shutdown_h(window);
	}

	void GRenderer::Draw2D(int localClientNum)
	{
		CG_DrawCrosshair_h(localClientNum);

		EventRenderer2D event;
		Application::Dispatch(event);
	}

	void GRenderer::Draw3D(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)
	{
		RB_EndSceneRendering_h(cmd, viewInfo, src, buf);
		GDraw3D::Render();

		EventRenderer3D event;
		Application::Dispatch(event);
	}

	void GRenderer::Commands(void* cmds)
	{
		// HLSL offline gameTime constants
		if (client_ui->connectionState != CA_ACTIVE)
			R_SetGameTime(gfx_cmdBufSourceState, UI::Time());
	}

	void GRenderer::SetSampler(GfxImage* image, int samplerIndex, GfxCmdBufSourceState* source, GfxCmdBufState* state,
		char samplerState)
	{
		const std::string_view name = image->name;
		const auto texture = image->texture.map;

		if (name == "screen" && Browser::Open && Browser::Texture && Browser::Texture->Data)
		{
			std::scoped_lock lock(Browser::TextureMutex);
			const auto browserTexture = reinterpret_cast<IDirect3DTexture9*>(Browser::Texture->Data);
			image->texture.map = browserTexture;
		}
	}

	void GRenderer::Frame(IDirect3DDevice9* device)
	{
		Renderer::Frame();
		Console::Frame();
		IDirect3DDevice9_EndScene_h(device);
	}

	HRESULT GRenderer::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		HRESULT hr = device->TestCooperativeLevel();
		if (hr == D3DERR_DEVICELOST)
			return hr;

		Renderer::ShutdownAssets();
		ImGui_ImplAPI_InvalidateDeviceObjects();
		hr = IDirect3DDevice9_Reset_h(device, pPresentationParameters);
		ImGui_ImplAPI_CreateDeviceObjects();
		Renderer::InitializeAssets();
		return hr;
	}
}
