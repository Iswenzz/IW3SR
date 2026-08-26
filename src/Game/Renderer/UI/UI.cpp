#include "UI.hpp"

#include "About.hpp"
#include "Modules.hpp"
#include "Toolbar.hpp"

namespace IW3SR
{
	void GUI::Initialize()
	{
		UI::Add<UC::About>();
		UI::Add<UC::Modules>();
		UI::Add<UC::Toolbar>();
	}
}
