#include "Modules.hpp"

#include "Core/System/Environment.hpp"

namespace IW3SR
{
	void Modules::Remove(const std::string& id)
	{
		const auto& entry = Entries[id];
		entry->Serialize(Serialized[entry->ID]);
		entry->Release();
		Entries.erase(id);
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
