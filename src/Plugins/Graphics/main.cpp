#include "Game/Plugin.hpp"

#include "Modules/Tweaks.hpp"

PLUGIN void Initialize()
{
	UI::UpdateContext();

	Modules::Load<Tweaks>();
}

PLUGIN void Shutdown()
{
	Modules::Remove("sr.graphics.tweaks");
}
