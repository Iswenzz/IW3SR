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
		MenuFrame.Position = Browser::MenuPosition;
		MenuFrame.Size = Browser::MenuSize;

		Browser::Start();

		if (Browser::Open)
			Browser::SetURL(Dvar::Get<char*>("cef_url"));
	}

	void CEF::Release()
	{
		Browser::Stop();
	}

	void CEF::Menu()
	{
		Browser::Frame();
	}

	void CEF::OnExecuteCommand(EventClientCommand& event)
	{
		if (!Browser::Open)
			return;

		if (event.command.starts_with("cef_url"))
			Browser::SetURL(Dvar::Get<char*>("cef_url"));
	}
}
