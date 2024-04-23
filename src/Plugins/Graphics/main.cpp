#include "Game/Plugin.hpp"
#include "Settings/General.hpp"

PLUGIN void Initialize()
{
	UI::InitializeContext();

	Settings::Load<General>();
}

PLUGIN void Info(Plugin* plugin)
{
	plugin->SetInfos("sr.graphics", "Graphics");
}
