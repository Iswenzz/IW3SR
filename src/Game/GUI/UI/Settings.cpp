#include "Settings.hpp"
#include "Game/Renderer/Settings/Settings.hpp"

#include <set>

namespace IW3SR::UC
{
	Settings::Settings() : Window("Settings")
	{
		SetRect(-170, 20, 150, 100);
		SetRectAlignment(HORIZONTAL_RIGHT, VERTICAL_TOP);
	}

	void Settings::OnRender()
	{
		Begin();
		const float frameWidth = ImGui::GetWindowContentRegionMax().x - 30;
		std::set<std::string> groups;

		for (const auto& [_, current] : IW3SR::Settings::Entries)
		{
			if (std::ranges::find(groups, current->Group) != groups.end())
				continue;

			groups.insert(current->Group);
			if (!ImGui::CollapsingHeader(current->Group, true))
				continue;

			for (const auto& [_, entry] : IW3SR::Settings::Entries)
			{
				if (current->Group != entry->Group)
					continue;

				ImGui::Text(entry->Name.c_str());
				ImGui::SameLine(frameWidth);
				ImGui::Button(ICON_FA_GEAR, entry->ID + "menu", &entry->MenuWindow.Open);

				if (entry->MenuWindow.Open)
				{
					entry->MenuWindow.Begin();
					entry->Menu();
					entry->MenuWindow.End();
				}
			}
		}
		End();
	}
}
