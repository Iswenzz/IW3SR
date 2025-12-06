#include "Settings.hpp"

namespace IW3SR
{
	void Settings::Remove(const std::string& id)
	{
		const auto& entry = Entries[id];
		entry->Serialize(Serialized[entry->ID]);
		entry->Release();
		Entries.erase(id);
	}

	void Settings::Deserialize()
	{
		Environment::Load(Serialized, "settings.json");
	}

	void Settings::Serialize()
	{
		Environment::Save(Serialized, "settings.json");
	}

	void Settings::Dispatch(Event& event)
	{
		for (const auto& [_, entry] : Entries)
			entry->OnEvent(event);
	}
}
