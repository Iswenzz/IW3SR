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
		UI::CreateWindow<UC::About>();
		UI::CreateWindow<UC::Binds>();
		UI::CreateWindow<UC::Modules>();
		UI::CreateWindow<UC::Settings>();
		UI::CreateWindow<UC::Toolbar>();
	}
}
