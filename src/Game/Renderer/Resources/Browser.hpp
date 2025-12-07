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
		void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
		void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects,
			const void* buffer, int width, int height) override;

		IMPLEMENT_REFCOUNTING(BrowserRenderHandler);
	};

	class BrowserClient : public CefClient, public CefLifeSpanHandler
	{
	public:
		BrowserClient(CefRefPtr<BrowserRenderHandler> renderHandler);

		CefRefPtr<CefRenderHandler> GetRenderHandler() override;
		CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

	private:
		CefRefPtr<BrowserRenderHandler> RenderHandler;

		IMPLEMENT_REFCOUNTING(BrowserClient);
	};

	class Browser
	{
	public:
		static inline CefRefPtr<CefBrowser> Instance;
		static inline CefRefPtr<BrowserClient> Client;
		static inline Ref<Texture> Texture;
		static inline std::mutex TextureMutex;
		static inline bool Active = false;

		static void Initialize();
		static void Shutdown();
		static void Frame();
	};
}
