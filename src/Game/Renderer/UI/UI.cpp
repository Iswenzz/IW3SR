#include "UI.hpp"

#include "About.hpp"
#include "Modules.hpp"
#include "Settings.hpp"
#include "Toolbar.hpp"

namespace IW3SR
{
	void GUI::Initialize()
	{
		UI::Add<UC::About>();
		UI::Add<UC::Modules>();
		UI::Add<UC::Settings>();
		UI::Add<UC::Toolbar>();
	}
}
