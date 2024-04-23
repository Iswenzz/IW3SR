#include "Game/Plugin.hpp"

#include "Modules/FPS.hpp"
#include "Modules/Lagometer.hpp"
#include "Modules/Velocity.hpp"

PLUGIN void Initialize(Plugin* plugin)
{
	plugin->SetInfos("sr.player", "Player");
}

PLUGIN void Game()
{
	UI::InitializeContext();

	Modules::Load<FPS>();
	Modules::Load<Lagometer>();
	Modules::Load<Velocity>();
}
