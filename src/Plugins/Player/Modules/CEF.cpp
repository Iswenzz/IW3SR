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

	void CEF::OnExecuteCommand(EventClientCommand& event)
	{
		if (!Instance->Open)
			return;

		if (event.command.starts_with("cef_url"))
			Browser::SetURL(Instance, Dvar::Get<char*>("cef_url"));
	}

	void CEF::OnRender()
	{
		if (Instance)
			Instance->Show = UI::Open && MenuFrame.Open;

		Browser::Frame();
	}
}
