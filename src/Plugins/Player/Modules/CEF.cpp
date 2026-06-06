#include "CEF.hpp"

namespace IW3SR::Addons
{
	CEF::CEF() : Module("sr.player.cef", "Player", "Browser")
	{
		MenuFrame.SetFlags(ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoTitleBar);
	}

	void CEF::Initialize()
	{
		Instance = Browser::Add("browser", Dvar::Get<char*>("cef_url"), { 20, 20 }, { 500, 300 });
		MenuFrame.Position = Instance->Position;
		MenuFrame.Size = Instance->Size;
	}

	void CEF::Release()
	{
		Browser::Remove("browser");
		Instance = nullptr;
	}

	void CEF::OnRender()
	{
		static std::string prevUrl = "";

		if (Instance)
			Instance->Show = UI::Open && MenuFrame.Open;

		const auto url = Dvar::Get<char*>("cef_url");
		if (Instance && Instance->Open && url != prevUrl)
			Browser::SetURL(Instance, url);
		prevUrl = url;

		Browser::Frame();
	}
}
