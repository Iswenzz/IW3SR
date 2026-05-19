#include "Base.hpp"

#include "Graphics/Modules/Texts.hpp"
#include "Graphics/Modules/Tweaks.hpp"

PLUGIN void Initialize()
{
	UI::UpdateContext();

	Modules::Load<Texts>();
	Modules::Load<Tweaks>();
}

PLUGIN void Shutdown()
{
	Modules::Remove("sr.graphics.texts");
	Modules::Remove("sr.graphics.tweaks");
}
