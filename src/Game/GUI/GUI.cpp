#include "GUI.hpp"

#include "Core/System/Environment.hpp"

namespace IW3SR
{
	GUI::GUI()
	{
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
		if (!UI::Get().Open)
			return;

		Toolbar.Render();
		Binds.Render();
		Modules.Render();
		Settings.Render();
		About.Render();
	}
}
