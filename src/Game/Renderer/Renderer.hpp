#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Renderer class.
	/// </summary>
	class GRenderer
	{
	public:
		/// <summary>
		/// Initialize the renderer.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Shutdown the renderer.
		/// </summary>
		/// <param name="window">The window.</param>
		static void Shutdown(int window);

		/// <summary>
		/// Draw 2D.
		/// </summary>
		/// <param name="localClientNum">Local client num.</param>
		static void Draw2D(int localClientNum);

		/// <summary>
		/// Draw 3D.
		/// </summary>
		/// <param name="cmd">Render command.</param>
		/// <param name="viewInfo">View information for the graphics.</param>
		/// <param name="src">Source state for the graphics command buffer.</param>
		/// <param name="buf">Graphics command buffer state.</param>
		static void Draw3D(GfxCmdBufInput* cmd, GfxViewInfo* viewInfo, GfxCmdBufSourceState* src, GfxCmdBufState* buf);

		/// <summary>
		/// Render commands.
		/// </summary>
		/// <param name="cmds">The commands.</param>
		static void Commands(void* cmds);

		/// <summary>
		/// Render frame.
		/// </summary>
		/// <param name="device">The device.</param>
		static void STDCALL Frame(IDirect3DDevice9* device);

		/// <summary>
		/// Reset device.
		/// </summary>
		/// <param name="device">The device.</param>
		/// <param name="pPresentationParameters">The presentation parameters.</param>
		/// <returns></returns>
		static HRESULT STDCALL Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);
	};
}
