#pragma once
#include "Game/Base.hpp"

#include "UI/About.hpp"
#include "UI/Binds.hpp"
#include "UI/Modules.hpp"
#include "UI/Settings.hpp"
#include "UI/Toolbar.hpp"

namespace IW3SR
{
	/// <summary>
	/// GUI class.
	/// </summary>
	class API GUI
	{
		CLASS_SINGLETON(GUI)
	public:
		UI& UI;

		UC::About About;
		UC::Binds Binds;
		UC::Modules Modules;
		UC::Settings Settings;
		UC::Toolbar Toolbar;

		/// <summary>
		/// Initialize GUI.
		/// </summary>
		void Initialize();

		/// <summary>
		/// Release GUI.
		/// </summary>
		void Release();

		/// <summary>
		/// Render frame.
		/// </summary>
		void Render();

	private:
		/// <summary>
		/// Initialize the GUI.
		/// </summary>
		GUI();
		virtual ~GUI() = default;

		SERIALIZE(GUI, About, Binds, Modules, Settings, Toolbar)
	};
}
