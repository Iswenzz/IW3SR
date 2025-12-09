#include "Base.hpp"

#include "Modules/CEF.hpp"
#include "Modules/CGAZ.hpp"
#include "Modules/FPS.hpp"
#include "Modules/KMOV.hpp"
#include "Modules/Lagometer.hpp"
#include "Modules/Movements.hpp"
#include "Modules/Velocity.hpp"

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
