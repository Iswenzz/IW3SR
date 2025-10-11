#include "Core/System/Crash.hpp"
#include "Core/System/Environment.hpp"
#include "Core/System/System.hpp"

#include "Client/Client.hpp"
#include "Renderer/Modules/Modules.hpp"
#include "Renderer/Settings/Settings.hpp"

#include "System/Console.hpp"
#include "System/Dvar.hpp"
#include "System/Patch.hpp"

void Application::Start()
{
	Crash::Setup();
	Environment::Binary();
	GConsole::Initialize();
	Plugins::Load();

	Dvar::Initialize();
	Patch::Initialize();
	Client::Initialize();
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
