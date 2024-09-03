#include "GUI.hpp"

#include "UI/About.hpp"
#include "UI/Binds.hpp"
#include "UI/Modules.hpp"
#include "UI/Settings.hpp"
#include "UI/Toolbar.hpp"

#include "Core/System/Environment.hpp"

namespace IW3SR
{
	void GUI::Initialize()
	{
		KeyOpen = Keyboard(Key_F10);

		UI::CreateWindow<UC::About>();
		UI::CreateWindow<UC::Binds>();
		UI::CreateWindow<UC::Modules>();
		UI::CreateWindow<UC::Settings>();
		UI::CreateWindow<UC::Toolbar>();
	}

	void GUI::Shutdown() { }

	void GUI::Frame()
	{
		if (Keyboard::IsPressed(Key_Escape))
			UI::Open = false;

		if (KeyOpen.IsPressed())
			UI::Open = !UI::Open;

		UI::Windows["Toolbar"]->Open = UI::Open;
	}
}
