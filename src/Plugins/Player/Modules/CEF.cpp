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
		MenuFrame.Position = { 20, 20 };
		MenuFrame.Size = { 500, 300 };

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
		if (!Browser::Open || !Browser::Texture || !Browser::Texture->Data)
			return;

		vec2 relative = { Mouse::Position.x - MenuFrame.RenderPosition.x,
			Mouse::Position.y - MenuFrame.RenderPosition.y };

		if (relative.x >= 0 && relative.x < MenuFrame.RenderSize.x && relative.y >= 0
			&& relative.y < MenuFrame.RenderSize.y)
		{
			CefMouseEvent mouseEvent;
			mouseEvent.x = static_cast<int>(relative.x * Browser::Size.x / MenuFrame.RenderSize.x);
			mouseEvent.y = static_cast<int>(relative.y * Browser::Size.y / MenuFrame.RenderSize.y);

			Browser::Instance->GetHost()->SendMouseMoveEvent(mouseEvent, false);
			if (Input::IsDown(Button_Left))
				Browser::Instance->GetHost()->SendMouseClickEvent(mouseEvent, MBT_LEFT, false, 1);
			if (Input::IsUp(Button_Left))
				Browser::Instance->GetHost()->SendMouseClickEvent(mouseEvent, MBT_LEFT, true, 1);
			if (Input::IsDown(Button_Right))
				Browser::Instance->GetHost()->SendMouseClickEvent(mouseEvent, MBT_RIGHT, false, 1);
			if (Input::IsUp(Button_Right))
				Browser::Instance->GetHost()->SendMouseClickEvent(mouseEvent, MBT_RIGHT, true, 1);
			if (Mouse::ScrollDelta)
				Browser::Instance->GetHost()->SendMouseWheelEvent(mouseEvent, 0, Mouse::ScrollDelta * 120);
			Mouse::ScrollDelta = 0;
		}
		std::scoped_lock lock(Browser::TextureMutex);
		Draw2D::Rect(Browser::Texture, MenuFrame.RenderPosition, MenuFrame.RenderSize, vec4(1));
	}

	void CEF::OnExecuteCommand(EventClientCommand& event)
	{
		if (!Browser::Open)
			return;

		if (event.command.starts_with("cef_url"))
			Browser::SetURL(Dvar::Get<char*>("cef_url"));
	}
}
