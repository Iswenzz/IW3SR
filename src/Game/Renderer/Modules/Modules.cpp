#include "Modules.hpp"
#include "Core/System/Environment.hpp"

#include <fstream>

namespace IW3SR
{
	void Modules::Initialize()
	{
		Deserialize();
	}

	void Modules::Release()
	{
		Serialize();
		Entries.clear();
	}

	void Modules::Enable(const std::string& id)
	{
		const auto& entry = Entries[id];
		entry->Initialize();
		entry->Deserialize(Serialized[entry->ID]);
		entry->IsEnabled = true;
	}

	void Modules::Disable(const std::string& id)
	{
		const auto& entry = Entries[id];
		entry->IsEnabled = false;
		entry->Serialize(Serialized[entry->ID]);
		entry->Release();
	}

	void Modules::Remove(const std::string& id)
	{
		if (auto it = Entries.find(id); it != Entries.end())
			Entries.erase(it);
	}

	void Modules::Deserialize()
	{
		IZ_ASSERT(Environment::Initialized, "Environment not initialized.");

		std::ifstream file(Environment::AppDirectory / "modules.json");
		if (file.peek() != std::ifstream::traits_type::eof())
			Serialized = nlohmann::json::parse(file);

		for (const auto& [_, entry] : Entries)
		{
			entry->Initialize();
			entry->Deserialize(Serialized[entry->ID]);
		}
	}

	void Modules::Serialize()
	{
		IZ_ASSERT(Environment::Initialized, "Environment not initialized.");

		for (const auto& [_, entry] : Entries)
		{
			entry->Serialize(Serialized[entry->ID]);
			entry->Release();
		}
		std::ofstream file(Environment::AppDirectory / "modules.json");
		file << Serialized.dump(4);
		file.close();
	}

	void Modules::Dispatch(Event& event)
	{
		for (const auto& [_, entry] : Entries)
			entry->OnEvent(event);
	}
}
