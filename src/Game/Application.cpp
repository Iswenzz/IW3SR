#include "Core/System/Crash.hpp"
#include "Core/System/Environment.hpp"
#include "Core/System/Plugins.hpp"
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
	Environment::Load();

	Dvar::Initialize();
	Patch::Initialize();

	if (System::IsDebug())
		GConsole::Initialize();

	Client::Initialize();
	Plugins::Initialize();
}

void Application::Shutdown()
{
	Environment::Save();

	if (System::IsDebug())
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
