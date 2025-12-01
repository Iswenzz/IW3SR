#pragma once
#include "Setting.hpp"

namespace IW3SR
{
	class API Settings
	{
	public:
		static inline std::map<std::string, Ref<Setting>> Entries;
		static inline nlohmann::json Serialized;

		template <class T = Setting>
		static void Load()
		{
			auto entry = CreateRef<T>();
			bool isSerialized = Serialized.contains(entry->ID);

			if (isSerialized)
				entry->Deserialize(Serialized[entry->ID]);
			entry->Initialize();

			Entries[entry->ID] = entry;
		}

		static void Remove(const std::string& id);
		static void Deserialize();
		static void Serialize();
		static void Dispatch(Event& event);
	};
}
