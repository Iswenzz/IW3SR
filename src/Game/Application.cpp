#include "Core/System/Environment.hpp"
#include "Core/System/Plugins.hpp"

#include "Client/Client.hpp"
#include "Renderer/Modules/Modules.hpp"
#include "Renderer/Settings/Settings.hpp"

#include "System/Console.hpp"
#include "System/Patch.hpp"
#include "System/Dvar.hpp"

void Application::Start()
{
	Environment::Initialize();
	Environment::Load();
	Dvar::Initialize();
		
	Patch::Initialize();
	GConsole::Initialize();
	Client::Initialize();
	Plugins::Initialize();
}

void Application::Shutdown()
{
	Environment::Save();

	GConsole::Shutdown();
	Plugins::Shutdown();
	Patch::Shutdown();
}

void Application::Dispatch(Event& event)
{
	Plugins::Dispatch(event);
	Modules::Dispatch(event);
	Settings::Dispatch(event);
	GConsole::Dispatch(event);
}
