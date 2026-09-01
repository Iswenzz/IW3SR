#include "CEF.hpp"

namespace IW3SR::Addons
{
	CEF::CEF() : Module("sr.player.cef", "Player", "Browser") { }

	void CEF::Initialize()
	{
		Instance = Browser::Add("browser", Dvar::Get<char*>("cef_url"), { 20, 20 }, { 500, 300 });
		Instance->Window->Name = "Browser";
		Instance->Window->Open = Interactive;
	}

	void CEF::Release()
	{
		Browser::Remove("browser");
		Instance = nullptr;
	}

	void CEF::Menu()
	{
		ImGui::Checkbox("Interactive", &Interactive);
		ImGui::Tooltip("Send the mouse and keyboard to the page while the menu is open");
	}

	void CEF::OnRender()
	{
		static std::string prevUrl = "";
		static bool prevHasTexture = false;

		static const auto cef_url = Dvar::Find("cef_url");

		if (Instance)
		{
			const bool hasTexture = !!Instance->Texture;

			// The page window closes itself from its title bar, which is the same as unticking it.
			// Only a window that was on screen last frame can have been closed; before that it is
			// simply one that was never opened.
			if (Instance->Show && !Instance->Window->Open)
				Interactive = false;

			Instance->Window->Open = Interactive;
			Instance->Show = UI::Open && Interactive;

			if (Instance->Open)
			{
				if (cef_url->current.string != prevUrl)
					Browser::SetURL(Instance, cef_url->current.string);
				if (hasTexture != prevHasTexture)
					GRenderer::UpdateMaterials();
			}
			prevUrl = cef_url->current.string;
			prevHasTexture = hasTexture;
		}
		Browser::Frame();
	}
}
