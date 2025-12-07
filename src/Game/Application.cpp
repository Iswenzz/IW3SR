#include "Base.hpp"

#include "Renderer/Modules/Modules.hpp"
#include "Renderer/Modules/Settings.hpp"

#include "System/Client.hpp"
#include "System/Console.hpp"
#include "System/Dvar.hpp"
#include "System/Patch.hpp"

void Application::Start()
{
	Crash::Setup();
	Environment::Binary();

	GConsole::Initialize();
	Dvar::Initialize();
	Patch::Initialize();
	Client::Initialize();

	Plugins::Load();
}

void Application::Shutdown()
{
	Plugins::Free();

	Client::Shutdown();
	GConsole::Shutdown();
}

void Application::Dispatch(Event& event)
{
	UI::Dispatch(event);
	Modules::Dispatch(event);
	Settings::Dispatch(event);
}
