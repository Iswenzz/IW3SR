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

	Dvar::Initialize();
	Patch::Initialize();
	Client::Initialize();

	if (System::IsDebug())
		GConsole::Initialize();
}

void Application::Shutdown()
{
	if (System::IsDebug())
		GConsole::Shutdown();

	Patch::Shutdown();
}

void Application::Dispatch(Event& event)
{
	UI::Dispatch(event);
	Modules::Dispatch(event);
	Settings::Dispatch(event);
}
