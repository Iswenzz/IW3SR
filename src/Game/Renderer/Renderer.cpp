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

	void GRenderer::UpdateMaterials()
	{
		for (int i = 0; i < rgp->materialCount; i++)
		{
			const auto material = rgp->sortedMaterials[i];
			const std::string_view name = material->info.name;

			if (name == "mc/sr_screen" && Browser::Open && Browser::Texture && Browser::Texture->Data)
			{
				material->textureTable[0].u.image->texture.map =
					reinterpret_cast<IDirect3DTexture9*>(Browser::Texture->Data);
			}
		}
		for (int i = 0; i < rgp->world->reflectionProbeCount; i++)
		{
			const auto probe = &rgp->world->reflectionProbes[i];
			if (IsRedCubemap(probe->reflectionImage->texture.cubemap))
				probe->reflectionImage->texture.cubemap = rgp->blackImageCube->texture.cubemap;
		}
	}

	bool GRenderer::IsRedCubemap(IDirect3DCubeTexture9* cubemap)
	{
		D3DSURFACE_DESC desc;
		if (FAILED(cubemap->GetLevelDesc(0, &desc)))
			return false;

		IDirect3DSurface9* surface = nullptr;
		if (FAILED(cubemap->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface)))
			return false;

		D3DLOCKED_RECT lockedRect;
		if (FAILED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY)))
		{
			surface->Release();
			return false;
		}
		const unsigned int* pixelData = (unsigned int*)lockedRect.pBits;
		unsigned int pixel = pixelData[0];
		unsigned char r = (pixel >> 16) & 0xFF;

		surface->UnlockRect();
		surface->Release();

		return r == 0xFF;
	}
}
