#include "Game/Plugin.hpp"

#include "Settings/Tweaks.hpp"

PLUGIN void Initialize()
{
	UI::InitializeContext();

	Settings::Load<Tweaks>();
}

PLUGIN void Shutdown()
{
	Settings::Remove("sr.graphics.tweaks");
}

PLUGIN void Info(Plugin* plugin)
{
	plugin->SetInfos("sr.graphics", "Graphics");
}
