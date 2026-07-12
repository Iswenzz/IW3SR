#include "Modules.hpp"

namespace IW3SR
{
	void Modules::Remove(const std::string& id)
	{
		const auto it = Entries.find(id);
		if (it == Entries.end())
			return;

		const auto& entry = it->second;
		entry->Serialize(Serialized[entry->ID]);
		entry->Release();
		Entries.erase(it);
	}

	void Modules::Deserialize()
	{
		Environment::Load(Serialized, "modules.json");
	}

	void Modules::Serialize()
	{
		Environment::Save(Serialized, "modules.json");
	}

	void Modules::Dispatch(Event& event)
	{
		for (const auto& [_, entry] : Entries)
			entry->OnEvent(event);
	}
}
