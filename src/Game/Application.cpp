#include "Base.hpp"

#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/System/Console.hpp"
#include "Game/System/Patch.hpp"

void Application::Start()
{
	Crash::Setup();
	Environment::Binary();
	Patch::Initialize();
}

void Application::LateStart()
{
	LateStarted = true;

	ThreadPool::Initialize();
	GConsole::Initialize();
	Plugins::Load();
}

void Application::Shutdown()
{
	if (!LateStarted)
		return;

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
