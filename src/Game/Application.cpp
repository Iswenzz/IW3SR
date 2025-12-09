#include "Base.hpp"

#include "Renderer/Modules/Modules.hpp"
#include "Renderer/Modules/Settings.hpp"

#include "System/Console.hpp"
#include "System/Patch.hpp"

void Application::Start()
{
	Crash::Setup();
	Environment::Binary();

	GConsole::Initialize();
	Patch::Initialize();
	Plugins::Load();
}

void Application::Shutdown()
{
	Plugins::Free();
	GConsole::Shutdown();
}

void Application::Dispatch(Event& event)
{
	UI::Dispatch(event);
	Modules::Dispatch(event);
	Settings::Dispatch(event);
}
