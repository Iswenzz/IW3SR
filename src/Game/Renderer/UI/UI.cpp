#include "UI.hpp"

#include "About.hpp"
#include "Main.hpp"

namespace IW3SR
{
	void GUI::Initialize()
	{
		UC::About::Initialize();
		UI::Add<UC::Main>();
	}
}
