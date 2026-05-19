#include "Base.hpp"

#include "Player/Modules/CEF.hpp"
#include "Player/Modules/CGAZ.hpp"
#include "Player/Modules/FPS.hpp"
#include "Player/Modules/KMOV.hpp"
#include "Player/Modules/Lagometer.hpp"
#include "Player/Modules/Movements.hpp"
#include "Player/Modules/Velocity.hpp"

PLUGIN void Initialize()
{
	UI::UpdateContext();

	Modules::Load<CEF>();
	Modules::Load<CGAZ>();
	Modules::Load<FPS>();
	Modules::Load<KMOV>();
	Modules::Load<Lagometer>();
	Modules::Load<Movements>();
	Modules::Load<Velocity>();
}

PLUGIN void Shutdown()
{
	Modules::Remove("sr.player.cef");
	Modules::Remove("sr.player.cgaz");
	Modules::Remove("sr.player.fps");
	Modules::Remove("sr.player.kmov");
	Modules::Remove("sr.player.lagometer");
	Modules::Remove("sr.player.movements");
	Modules::Remove("sr.player.velocity");
}
