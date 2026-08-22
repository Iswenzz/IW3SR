#include "Base.hpp"

#include "Graphics/Modules/General.hpp"
#include "Graphics/Modules/Tweaks.hpp"

PLUGIN void Initialize()
{
	UI::UpdateContext();

	Modules::Load<General>();
	Modules::Load<Tweaks>();
}

PLUGIN void Shutdown()
{
	Modules::Remove("sr.graphics.general");
	Modules::Remove("sr.graphics.tweaks");
}
