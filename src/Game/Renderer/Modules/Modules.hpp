#pragma once
#include "Module.hpp"

namespace IW3SR
{
	/// <summary>
	/// Modules class.
	/// </summary>
	class API Modules
	{
	public:
		static inline std::unordered_map<std::string, Ref<Module>> Entries;
		static inline nlohmann::json Serialized;

		/// <summary>
		/// Load a module.
		/// </summary>
		/// <typeparam name="T">The module type.</typeparam>
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

		/// <summary>
		/// Remove a module.
		/// </summary>
		/// <param name="id">The module id.</param>
		static void Remove(const std::string& id);

		/// <summary>
		/// Load the modules.
		/// </summary>
		static void Deserialize();

		/// <summary>
		/// Serialize the modules.
		/// </summary>
		static void Serialize();

		/// <summary>
		/// Dipatch event.
		/// </summary>
		/// <param name="event">The event.</param>
		static void Dispatch(Event& event);
	};
}
