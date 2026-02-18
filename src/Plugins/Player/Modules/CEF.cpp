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

		std::scoped_lock lock(Browser::TextureMutex);
		Draw2D::Rect(Browser::Texture, MenuFrame.RenderPosition, MenuFrame.RenderSize, vec4(1));
		auto host = Browser::Instance->GetHost();

		uint32_t modifiers = 0;
		if (Input::IsDown(Key_Ctrl) || Input::IsDown(Key_RightCtrl))
			modifiers |= EVENTFLAG_CONTROL_DOWN;
		if (Input::IsDown(Key_Shift) || Input::IsDown(Key_RightShift))
			modifiers |= EVENTFLAG_SHIFT_DOWN;
		if (Input::IsDown(Key_Alt) || Input::IsDown(Key_RightAlt))
			modifiers |= EVENTFLAG_ALT_DOWN;

		vec2 relative = Mouse::Position - MenuFrame.RenderPosition;
		if (Math::Contains(relative, MenuFrame.RenderSize))
		{
			CefMouseEvent mouseEvent;
			mouseEvent.x = static_cast<int>(relative.x / MenuFrame.RenderSize.x * Browser::Size.x);
			mouseEvent.y = static_cast<int>(relative.y / MenuFrame.RenderSize.y * Browser::Size.y);
			mouseEvent.modifiers = modifiers;

			host->SendMouseMoveEvent(mouseEvent, false);

			if (Input::IsDown(Button_Left))
				host->SendMouseClickEvent(mouseEvent, MBT_LEFT, false, 1);
			if (Input::IsUp(Button_Left))
				host->SendMouseClickEvent(mouseEvent, MBT_LEFT, true, 1);
			if (Input::IsDown(Button_Right))
				host->SendMouseClickEvent(mouseEvent, MBT_RIGHT, false, 1);
			if (Input::IsUp(Button_Right))
				host->SendMouseClickEvent(mouseEvent, MBT_RIGHT, true, 1);
			if (Mouse::ScrollDelta)
				host->SendMouseWheelEvent(mouseEvent, 0, Mouse::ScrollDelta * 120);
		}
		for (auto& [id, info] : Input::Inputs)
		{
			if (id == Button_Left || id == Button_Right || id == Button_Middle)
				continue;
			if (!Input::IsDown(id) && !Input::IsUp(id))
				continue;

			CefKeyEvent keyEvent;
			keyEvent.modifiers = modifiers;
			keyEvent.windows_key_code = info.OS;
			keyEvent.native_key_code = info.OS;

			if (Input::IsPressed(id))
			{
				keyEvent.type = KEYEVENT_RAWKEYDOWN;
				host->SendKeyEvent(keyEvent);

				if (Keyboard::Char)
				{
					keyEvent.type = KEYEVENT_CHAR;
					keyEvent.windows_key_code = Keyboard::Char;
					keyEvent.character = Keyboard::Char;
					host->SendKeyEvent(keyEvent);
				}
			}
			if (Input::IsUp(id))
			{
				keyEvent.type = KEYEVENT_KEYUP;
				host->SendKeyEvent(keyEvent);
			}
		}
	}

	void CEF::OnExecuteCommand(EventClientCommand& event)
	{
		if (!Browser::Open)
			return;

		if (event.command.starts_with("cef_url"))
			Browser::SetURL(Dvar::Get<char*>("cef_url"));
	}
}
