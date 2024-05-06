#include "GUI.hpp"

#include "Core/System/Environment.hpp"

namespace IW3SR
{
	GUI::GUI()
	{
		KeyOpen = Keyboard(Key_F10);

		About = UC::About();
		Binds = UC::Binds();
		Modules = UC::Modules();
		Settings = UC::Settings();
		Toolbar = UC::Toolbar();
	}

	void GUI::Initialize()
	{
		Environment::Deserialize("GUI", *this);
	}

	void GUI::Shutdown()
	{
		Environment::Serialize("GUI", *this);
	}

	void GUI::Render()
	{
		auto& UI = UI::Get();

		if (KeyOpen.IsPressed())
			UI.Open = !UI.Open;

		if (Keyboard::IsPressed(Key_Escape))
			UI.Open = false;

		if (UI.Open)
		{
			Toolbar.Render();
			Binds.Render();
			Modules.Render();
			Settings.Render();
			About.Render();
		}
	}
}
