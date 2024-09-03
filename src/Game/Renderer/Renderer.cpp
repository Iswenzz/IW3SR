#include "Renderer.hpp"

#include "Drawing/Draw2D.hpp"
#include "Drawing/Draw3D.hpp"
#include "Modules/Modules.hpp"
#include "Settings/Settings.hpp"

#include "Game/GUI/GUI.hpp"
#include "Game/System/Console.hpp"

namespace IW3SR
{
	void GRenderer::Initialize()
	{
		R_Init_h();

		Device::Swap(dx->d3d9, dx->device);
		Renderer::Initialize();
		GUI::Initialize();

		Settings::Deserialize();
		Modules::Deserialize();
		Plugins::Initialize();
	}

	void GRenderer::Shutdown(int window)
	{
		Plugins::Shutdown();
		Modules::Serialize();
		Settings::Serialize();

		GUI::Shutdown();
		Renderer::Shutdown();

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

	void GRenderer::Frame()
	{
		Actions::Submit();
		GUI::Frame();

		Renderer::Begin();

		EventRendererRender event;
		Application::Dispatch(event);

		Renderer::End();
	}
}
