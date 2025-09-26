#include "GUI.hpp"

#include "UI/About.hpp"
#include "UI/Binds.hpp"
#include "UI/Modules.hpp"
#include "UI/Settings.hpp"
#include "UI/Toolbar.hpp"

namespace IW3SR
{
	void GUI::Initialize()
	{
		UI::Add<UC::About>();
		UI::Add<UC::Binds>();
		UI::Add<UC::Modules>();
		UI::Add<UC::Settings>();
		UI::Add<UC::Toolbar>();
	}
}
