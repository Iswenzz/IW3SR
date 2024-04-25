#include "Game/Plugin.hpp"

#include "Modules/FPS.hpp"
#include "Modules/Movements.hpp"
#include "Modules/Lagometer.hpp"
#include "Modules/Velocity.hpp"

PLUGIN void Initialize()
{
	UI::InitializeContext();

	Modules::Load<FPS>();
	Modules::Load<Lagometer>();
	Modules::Load<Movements>();
	Modules::Load<Velocity>();
}

PLUGIN void Info(Plugin* plugin)
{
	plugin->SetInfos("sr.player", "Player");
}
