#pragma once
#include "Game/Base.hpp"

#include <cef_app.h>
#include <cef_browser.h>
#include <cef_client.h>

namespace IW3SR
{
	class BrowserRenderHandler : public CefRenderHandler
	{
	public:
		Ref<Texture> Texture;
		int LastWidth = 0;
		int LastHeight = 0;

		void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override
		{
			rect = CefRect(0, 0, 1280, 720);
		}

		void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects,
			const void* buffer, int width, int height) override
		{
			if (!buffer)
				return;

			if (!Texture || width != LastWidth || height != LastHeight)
			{
				Texture = Texture::Create("browser", { width, height });
				LastWidth = width;
				LastHeight = height;
			}
			IDirect3DTexture9* texture = reinterpret_cast<IDirect3DTexture9*>(Texture->Data);
			D3DLOCKED_RECT rect;
			texture->LockRect(0, &rect, nullptr, 0);
			memcpy(rect.pBits, buffer, width * height * 4);
			texture->UnlockRect(0);
		}

		IMPLEMENT_REFCOUNTING(BrowserRenderHandler);
	};

	class BrowserClient : public CefClient, public CefLifeSpanHandler
	{
	public:
		BrowserClient(CefRefPtr<BrowserRenderHandler> renderHandler) : RenderHandler(renderHandler) { }

		CefRefPtr<CefRenderHandler> GetRenderHandler() override
		{
			return RenderHandler;
		}

		CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
		{
			return this;
		}

		void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
		{
			CefQuitMessageLoop();
		}

	private:
		CefRefPtr<BrowserRenderHandler> RenderHandler;

		IMPLEMENT_REFCOUNTING(BrowserClient);
	};

	class Browser
	{
	public:
		static inline CefRefPtr<BrowserClient> Client;

		static void Initialize();
		static void Shutdown();
	};
}
