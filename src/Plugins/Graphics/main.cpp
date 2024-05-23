#include "Game/Plugin.hpp"

#include "Settings/Tweaks.hpp"

PLUGIN void Initialize()
{
	UI::InitializeContext();

	Settings::Load<Tweaks>();
}

PLUGIN void Info(Plugin* plugin)
{
	plugin->SetInfos("sr.graphics", "Graphics");
}
