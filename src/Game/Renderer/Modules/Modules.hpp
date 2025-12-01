#pragma once
#include "Module.hpp"

namespace IW3SR
{
	class API Modules
	{
	public:
		static inline std::map<std::string, Ref<Module>> Entries;
		static inline nlohmann::json Serialized;

		template <class T = Module>
		static void Load()
		{
			auto entry = CreateRef<T>();
			bool isSerialized = Serialized.contains(entry->ID);

			if (isSerialized)
				entry->Deserialize(Serialized[entry->ID]);

			if (entry->IsEnabled)
				entry->Initialize();

			Entries[entry->ID] = entry;
		}

		static void Remove(const std::string& id);
		static void Deserialize();
		static void Serialize();
		static void Dispatch(Event& event);
	};
}
