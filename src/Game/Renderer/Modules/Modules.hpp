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
		static inline std::map<std::string, Ref<Module>> Entries;
		static inline nlohmann::json Serialized;

		/// <summary>
		/// Initialize the modules.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Release the modules.
		/// </summary>
		static void Release();

		/// <summary>
		/// Load a module.
		/// </summary>
		/// <typeparam name="M">The module type.</typeparam>
		/// <param name="enabled">Is enabled by default.</param>
		template <class M = Module>
		static void Load(bool enabled = true)
		{
			auto entry = CreateRef<M>();
			bool isSerialized = Serialized.contains(entry->ID);

			entry->IsEnabled = enabled;
			if (isSerialized)
				entry->Deserialize(Serialized[entry->ID]);

			Entries.insert({ entry->ID, std::move(entry) });
		}

		/// <summary>
		/// Enable a module.
		/// </summary>
		/// <param name="id">The module id.</param>
		static void Enable(const std::string& id);

		/// <summary>
		/// Disable a module.
		/// </summary>
		/// <param name="id">The module id.</param>
		static void Disable(const std::string& id);

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
