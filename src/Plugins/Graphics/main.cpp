#include "Game/Plugin.hpp"

#include "Modules/Tweaks.hpp"

PLUGIN void Initialize()
{
	UI::InitializeContext();

	Modules::Load<Tweaks>();
}

PLUGIN void Shutdown()
{
	Modules::Remove("sr.graphics.tweaks");
}

PLUGIN void Info(Plugin* plugin)
{
	plugin->SetInfos("sr.graphics", "Graphics");
}
