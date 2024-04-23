#include "Renderer.hpp"
#include "GUI.hpp"

#include "Drawing/Draw2D.hpp"
#include "Drawing/Draw3D.hpp"
#include "Modules/Modules.hpp"
#include "Settings/Settings.hpp"

#include "Game/System/Console.hpp"

namespace IW3SR
{
	void GRenderer::Initialize()
	{
		R_Init_h();

		Device::Initialize(dx->d3d9, dx->device);
		Device::CreateScreen();
		Renderer::Initialize();

		GUI::Get().Initialize();
		Modules::Initialize();
		Settings::Initialize();
	}

	void GRenderer::Shutdown(int window)
	{
		Settings::Release();
		Modules::Release();
		GUI::Get().Release();

		Renderer::Shutdown();
		R_Shutdown_h(window);
	}

	void GRenderer::Draw2D(int localClientNum)
	{
		CG_DrawCrosshair_h(localClientNum);

		EventRenderer2D event;
		Application::Get().Dispatch(event);
	}

	void GRenderer::Draw3D(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf)
	{
		RB_EndSceneRendering_h(cmd, viewInfo, src, buf);
		GDraw3D::Render();

		EventRenderer3D event;
		Application::Get().Dispatch(event);
	}

	void GRenderer::Commands(void* cmds)
	{
		// HLSL offline gameTime constants
		if (client_ui->connectionState != CA_ACTIVE)
			R_SetGameTime(gfx_cmdBufSourceState, UI::Get().Time());
	}

	void GRenderer::Render()
	{
		GConsole::Frame();
		Renderer::Begin();
		GUI::Get().Render();

		if (client_ui->connectionState == CA_ACTIVE)
		{
			EventRendererRender event;
			Application::Get().Dispatch(event);
		}
		Renderer::End();
		Keyboard::Reset();
	}
}
