#pragma once
#include "Setting.hpp"

namespace IW3SR
{
	/// <summary>
	/// Engine settings.
	/// </summary>
	class API Settings
	{
	public:
		static inline std::map<std::string, Ref<Setting>> Entries;
		static inline nlohmann::json Serialized;

		/// <summary>
		/// Initialize the settings.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Release the settings.
		/// </summary>
		static void Release();

		/// <summary>
		/// Load a setting.
		/// </summary>
		/// <typeparam name="F">The setting type.</typeparam>
		template <class F = Setting>
		static void Load()
		{
			auto entry = CreateRef<F>();
			bool isSerialized = Serialized.contains(entry->ID);

			if (isSerialized)
				entry->Deserialize(Serialized[entry->ID]);

			Entries.insert({ entry->ID, std::move(entry) });
		}

		/// <summary>
		/// Remove a setting.
		/// </summary>
		/// <param name="id">The setting id.</param>
		static void Remove(const std::string& id);

		/// <summary>
		/// Load the settings.
		/// </summary>
		static void Deserialize();

		/// <summary>
		/// Serialize the settings.
		/// </summary>
		static void Serialize();

		/// <summary>
		/// Dispatch event.
		/// </summary>
		/// <param name="event">The event.</param>
		static void Dispatch(Event& event);
	};
}
