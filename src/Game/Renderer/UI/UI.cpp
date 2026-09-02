#include "UI.hpp"

#include "Engine/Backend/ImGUI/UI/Consent.hpp"

#include "About.hpp"
#include "Main.hpp"

namespace IW3SR
{
	void GUI::Initialize()
	{
		UC::About::Initialize();
		UI::Add<UC::Main>();

		// The engine registers the prompt closed and leaves the moment to us. Here is the earliest
		// one that works: the renderer has just finished, so the frame exists and can be shown.
		IzEngine::UC::Consent::Prompt();
	}
}
