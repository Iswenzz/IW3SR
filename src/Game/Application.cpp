#include "Base.hpp"

#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/System/Console.hpp"
#include "Game/System/Patch.hpp"

void Application::Prepare()
{
	Environment::Binary();
	Patch::Initialize();
}

void Application::Initialize()
{
	Crash::Initialize();
	Browser::Initialize(true);
	ThreadPool::Initialize();
	GConsole::Initialize();
	Plugins::Load();
}

void Application::Shutdown()
{
	ThreadPool::Shutdown();
	Plugins::Free();
	GConsole::Shutdown();
	Crash::Shutdown();
}

void Application::Dispatch(Event& event)
{
	UI::Dispatch(event);
	Modules::Dispatch(event);
	GConsole::Dispatch(event);
}
