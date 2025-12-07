#include "Browser.hpp"

namespace IW3SR
{
	void Browser::Initialize()
	{
		if (Active)
			return;

		Log::WriteLine(Channel::Verbose, "Browser initialized");

		CefMainArgs args(GetModuleHandle(nullptr));
		int code = CefExecuteProcess(args, nullptr, nullptr);
		if (code >= 0)
			return;

		const auto pathResources = Environment::Path(Directory::Resources) / "cef";
		const auto pathLocales = Environment::Path(Directory::Resources) / "cef" / "locales";
		const auto pathCache = Environment::Path(Directory::App) / "cache";
		const auto pathLog = Environment::Path(Directory::App) / "logs" / "cef.log";
		const auto pathSubProcess = Environment::Path(Directory::Base) / "CEF.exe";

		CefSettings settings;
		settings.log_severity = LOGSEVERITY_ERROR;
		settings.multi_threaded_message_loop = true;
		settings.windowless_rendering_enabled = true;
		settings.no_sandbox = true;
		CefString(&settings.resources_dir_path).FromString(pathResources.string());
		CefString(&settings.locales_dir_path).FromString(pathLocales.string());
		CefString(&settings.cache_path).FromString(pathCache.string());
		CefString(&settings.root_cache_path).FromString(pathCache.string());
		CefString(&settings.log_file).FromString(pathLog.string());
		CefString(&settings.browser_subprocess_path).FromString(pathSubProcess.string());

		if (!CefInitialize(args, settings, nullptr, nullptr))
			return;

		CefBrowserSettings browserSettings;
		CefWindowInfo windowInfo;
		windowInfo.SetAsWindowless(nullptr);

		CefRefPtr<BrowserRenderHandler> renderHandler = new BrowserRenderHandler();
		Client = new BrowserClient(renderHandler);
		//const auto url = "https://youtu.be/UgQFcvYg9Kk?t=90";
		const auto url = "https://sr.iswenzz.com";
		CefBrowserHost::CreateBrowser(windowInfo, Client, url, browserSettings, nullptr, nullptr);

		Active = true;
	}

	void Browser::Shutdown()
	{
		//if (!Active)
			return;

		if (Instance)
		{
			Instance->GetHost()->CloseBrowser(true);
			Instance = nullptr;
		}
		Client = nullptr;
		Active = false;
		Log::WriteLine(Channel::Verbose, "Browser shutdown");
		CefShutdown();
	}

	void Browser::Frame()
	{
		if (!Texture)
			return;

		std::scoped_lock lock(TextureMutex);
		//Material* mtl = Material_RegisterHandle("black", 3);
		//if (!mtl)
			//return;

		//auto texture = reinterpret_cast<IDirect3DTexture9*>(Browser::Texture->Data);
		//mtl->textureTable->u.image->texture.map = texture;
		Draw2D::Rect(Texture, { 0, 0 }, Window::Size);
	}

	void BrowserRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
	{
		rect = CefRect(0, 0, Window::Size.x, Window::Size.y);
	}

	void BrowserRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects,
		const void* buffer, int width, int height)
	{
		if (!buffer)
			return;

		std::scoped_lock lock(Browser::TextureMutex);

		if (!Browser::Texture)
			Browser::Texture = Texture::Create("browser", Window::Size);

		IDirect3DTexture9* texture = reinterpret_cast<IDirect3DTexture9*>(Browser::Texture->Data);
		D3DLOCKED_RECT rect;
		texture->LockRect(0, &rect, nullptr, 0);
		memcpy(rect.pBits, buffer, width * height * 4);
		texture->UnlockRect(0);
	}

	BrowserClient::BrowserClient(CefRefPtr<BrowserRenderHandler> renderHandler)
	{
		RenderHandler = renderHandler;
	}

	CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler()
	{
		return RenderHandler;
	}

	CefRefPtr<CefLifeSpanHandler> BrowserClient::GetLifeSpanHandler()
	{
		return this;
	}

	void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
	{
		Browser::Instance = browser;
	}
}
