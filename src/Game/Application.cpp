#include "Base.hpp"

#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/System/Console.hpp"
#include "Game/System/Patch.hpp"

void Application::Start()
{
	Crash::Setup();
	Environment::Binary();

	ThreadPool::Initialize();
	GConsole::Initialize();
	Patch::Initialize();
	Plugins::Load();
}

void Application::Shutdown()
{
	Plugins::Free();
	GConsole::Shutdown();
	ThreadPool::Shutdown();
}

void Application::Dispatch(Event& event)
{
	UI::Dispatch(event);
	Modules::Dispatch(event);
	GConsole::Dispatch(event);
}
