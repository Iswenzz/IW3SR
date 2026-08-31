#include "Renderer.hpp"

#include "Game/Renderer/Drawing/Draw2D.hpp"
#include "Game/Renderer/Drawing/Draw3D.hpp"
#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/Renderer/Portal/Portal.hpp"
#include "Game/Renderer/UI/About.hpp"
#include "Game/Renderer/UI/UI.hpp"
#include "Game/System/Assets.hpp"
#include "Game/System/Capture.hpp"
#include "Game/System/Mouse.hpp"
#include "Game/System/System.hpp"
#include "Game/System/Timestep.hpp"

namespace IW3SR
{
	void GRenderer::Initialize()
	{
		R_Init_h();

		IDirect3DDevice9_Reset_h.Update(VTABLE(dx->device, 16));
		IDirect3DDevice9_EndScene_h.Update(VTABLE(dx->device, 42));

		Dvar::Initialize();
		Timestep::Initialize();
		Assets::Initialize();
		Capture::Initialize();
		GMouse::Initialize();
		GPortal::Initialize();
		Modules::Deserialize();

		DX9GraphicsContext::Swap(dx->d3d9, dx->device);
		Renderer::Initialize(RendererBackend::DX9);
		GUI::Initialize();
	}

	void GRenderer::Shutdown(int window)
	{
		Swaps.Clear();
		GPortal::Shutdown();

		Browser::Lock();
		Renderer::Shutdown();
		Modules::Serialize();
		Browser::Unlock();

		R_Shutdown_h(window);
	}

	void GRenderer::DrawViewpos()
	{
		if (Patch::UseCoD4X)
			return;

		static const auto enabled = Dvar::Find("debug_show_viewpos");
		if (!enabled || !enabled->current.enabled || !cgs || !clients)
			return;

		static GText text{ "", FONT_NORMAL, -10, 60, 1.2, vec4(1) };

		const vec3& origin = cgs->refdef.vieworg;
		const vec3& angles = clients->viewangles;

		text.Value = std::format("{:.1f} {:.1f} {:.1f}\n{:.1f} {:.1f} {:.1f}", origin.x, origin.y, origin.z, angles.x,
			angles.y, angles.z);

		text.SetRectAlignment(Horizontal::Right, Vertical::Fullscreen);
		text.SetAlignment(Alignment::Right, Alignment::Top);
		text.Render();
	}

	void GRenderer::Draw2D(int localClientNum)
	{
		CG_DrawCrosshair_h(localClientNum);
		GPortal::DrawDebug();
		DrawViewpos();

		EventRenderer2D event;
		Application::Dispatch(event);
	}

	void GRenderer::Draw3D(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)
	{
		RB_EndSceneRendering_h(cmd, viewInfo, src, buf);
		if (GPortal::Rendering)
			return;

		GDraw3D::Render();

		EventRenderer3D event;
		Application::Dispatch(event);
	}

	void GRenderer::DrawVersion()
	{
		constexpr size_t COD4X_VERSION_OFFSET = 8;
		constexpr size_t COD4X_VERSION_LENGTH = 4;

		constexpr float VERSION_MARGIN = 10.0f;

		static GText text{ "", FONT_NORMAL, -VERSION_MARGIN, 450, 1.4, vec4(1) };
		static GText update{ "", FONT_NORMAL, 10, 470, 1.4, vec4(0, 1, 1, 1) };

		if (text.Value.empty())
		{
			static const auto shortversion = Dvar::Find("shortversion");
			const std::string_view shortversionString = shortversion->current.string;

			text.Value = std::format("CoD4 {}\nIW3SR {}", shortversionString, APPLICATION_VERSION);
			if (COD4X_BASE)
			{
				static const auto version = Dvar::Find("version");
				const std::string_view versionString = version->current.string;

				const auto cod4xversion = versionString.size() >= COD4X_VERSION_OFFSET + COD4X_VERSION_LENGTH
					? versionString.substr(COD4X_VERSION_OFFSET, COD4X_VERSION_LENGTH)
					: versionString;

				text.Value =
					std::format("CoD4 {}\nCoD4x {}\nIW3SR {}", shortversionString, cod4xversion, APPLICATION_VERSION);
			}
			text.SetRectAlignment(Horizontal::Right, Vertical::Fullscreen);
			text.SetAlignment(Alignment::Right, Alignment::Top);
		}
		if (update.Value.empty() && UC::About::UpdateAvailable)
		{
			update.Value = "IW3SR update available";
			update.SetRectAlignment(Horizontal::Fullscreen, Vertical::Fullscreen);
		}
		text.Render();
		update.Render();
	}

	void GRenderer::ExecuteRenderCommandsLoop(void* cmds)
	{
		// HLSL offline gameTime constants
		if (client_ui->connectionState != CA_ACTIVE)
			R_SetGameTime(gfx_cmdBufSourceState, UI::Time());
	}

	void GRenderer::Frame(IDirect3DDevice9* device)
	{
		// An offscreen portal pass ends its own scene; the overlay belongs to the visible frame only.
		if (GPortal::Rendering)
		{
			IDirect3DDevice9_EndScene_h(device);
			return;
		}
		if (PendingMaterialUpdate)
		{
			PendingMaterialUpdate = false;
			UpdateMaterials();
		}
		if (!Capture::Recording || Capture::DrawOverlay())
		{
			Tasks.Submit();
			Renderer::Frame();
			Input::Reset();
		}

		Console::Frame();

		IDirect3DDevice9_EndScene_h(device);

		// The backbuffer can only be resolved outside of a begin/end scene block.
		Capture::Frame(device);
	}

	HRESULT GRenderer::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		DX9GraphicsContext::PresentParameters = *pPresentationParameters;

		HRESULT hr = device->TestCooperativeLevel();
		if (hr != D3D_OK && hr != D3DERR_DEVICENOTRESET)
			return IDirect3DDevice9_Reset_h(device, pPresentationParameters);

		Browser::Lock();
		Swaps.Clear();
		GPortal::Shutdown(); // hand the colour maps back before the render targets are released

		GPUResource::NotifyBeforeReset();
		ImGui_ImplAPI_InvalidateDeviceObjects();
		hr = IDirect3DDevice9_Reset_h(device, pPresentationParameters);

		if (SUCCEEDED(hr))
		{
			GPUResource::NotifyAfterReset();
			ImGui_ImplAPI_CreateDeviceObjects();
			PendingMaterialUpdate = true;
		}
		Browser::Unlock();
		return hr;
	}

	void GRenderer::UpdateMaterials()
	{
		if (!rgp->world)
			return;

		for (int i = 0; i < rgp->materialCount; i++)
		{
			const auto material = rgp->sortedMaterials[i];
			if (!material || !material->info.name)
				continue;

			const std::string_view name = material->info.name;

			if (name == "mc/sr_screen")
			{
				if (!material->textureCount || !material->textureTable || !material->textureTable[0].u.image)
					continue;

				auto instance = Browser::Get("browser");
				if (instance && instance->Open && instance->Texture)
				{
					auto dxTexture = std::static_pointer_cast<DX9Texture>(instance->Texture);
					Swaps.Add("sr_screen", reinterpret_cast<void**>(&material->textureTable[0].u.image->texture.map),
						dxTexture->Data);
				}
			}
		}
		for (int i = 0; i < rgp->world->reflectionProbeCount; i++)
		{
			const auto probe = &rgp->world->reflectionProbes[i];
			if (!probe->reflectionImage || !probe->reflectionImage->texture.cubemap)
				continue;

			if (IsRedCubemap(probe->reflectionImage->texture.cubemap))
			{
				Swaps.Add(std::format("probe_{}", i),
					reinterpret_cast<void**>(&probe->reflectionImage->texture.cubemap),
					rgp->blackImageCube->texture.cubemap);
			}
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

	void GRenderer::AddCmdDrawText(const char** text, int maxChars, Font_s* font, float x, float y, float xScale,
		float yScale, float rotation, int style, const vec4& color)
	{
		static std::string buffer;
		buffer = *text;

		EventRendererText event(buffer, font, { x, y }, { xScale, yScale }, color);
		Application::Dispatch(event);

		*text = buffer.data();
	}

	void GRenderer::AddCmdDrawTextWithEffects(const char* text, int maxChars, Font_s* font, float x, float y,
		float xScale, float yScale, float rotation, const vec4& color, int style, const vec4& glowColor,
		Material* fxMaterial, Material* fxMaterialGlow, int fxBirthTime, int fxLetterTime, int fxDecayStartTime,
		int fxDecayDuration)
	{
		static std::string buffer;
		buffer = text;

		EventRendererText event(buffer, font, { x, y }, { xScale, yScale }, color);
		Application::Dispatch(event);

		R_AddCmdDrawTextWithEffects_h(buffer.data(), maxChars, font, x, y, xScale, yScale, rotation, color, style,
			glowColor, fxMaterial, fxMaterialGlow, fxBirthTime, fxLetterTime, fxDecayStartTime, fxDecayDuration);
	}
}
