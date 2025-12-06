#include "Browser.hpp"

namespace IW3SR
{
	void Browser::Initialize()
	{
		CefMainArgs args(nullptr);
		int code = CefExecuteProcess(args, nullptr, nullptr);
		if (code >= 0)
			return;

		CefSettings settings;
		settings.log_severity = LOGSEVERITY_DEBUG;
		settings.windowless_rendering_enabled = true;
		CefString(&settings.browser_subprocess_path).FromASCII("bootstrap.exe");

		CefInitialize(args, settings, nullptr, nullptr);

		CefBrowserSettings browserSettings;
		CefWindowInfo windowInfo;
		windowInfo.SetAsWindowless(nullptr);

		CefRefPtr<BrowserRenderHandler> renderHandler = new BrowserRenderHandler();
		Client = new BrowserClient(renderHandler);

		CefBrowserHost::CreateBrowserSync(windowInfo, Client, "https://google.com", browserSettings, nullptr, nullptr);

		CefRunMessageLoop();
	}

	void Browser::Shutdown()
	{
		CefShutdown();
	}
}
