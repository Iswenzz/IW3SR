#include "Modules.hpp"

#include "Game/Renderer/Modules/Modules.hpp"

#include <set>

namespace IW3SR::UC
{
	Modules::Modules() : Frame("Modules")
	{
		SetRect(-170, 20, 150, 185);
		SetRectAlignment(Horizontal::Right, Vertical::Top);
	}

	void Modules::OnRender()
	{
		Begin();

		const float buttonWidth = ImGui::CalcTextSize(ICON_FA_GEAR).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float availWidth = ImGui::GetContentRegionAvail().x;

		std::set<std::string> groups;
		for (const auto& [_, current] : IW3SR::Modules::Entries)
		{
			if (std::ranges::find(groups, current->Group) != groups.end())
				continue;

			groups.insert(current->Group);
			if (!ImGui::CollapsingHeader(current->Group, true))
				continue;

			for (const auto& [_, entry] : IW3SR::Modules::Entries)
			{
				if (current->Group != entry->Group)
					continue;

				if (ImGui::Toggle(entry->ID + "toggle", &entry->IsEnabled))
					entry->IsEnabled ? entry->Initialize() : entry->Release();

				ImGui::SameLine();
				ImGui::Text(entry->Name.c_str());
				ImGui::SameLine(availWidth - buttonWidth);
				ImGui::Button(ICON_FA_GEAR, entry->ID + "menu", &entry->MenuFrame.Open);

				if (entry->MenuFrame.Open)
				{
					entry->MenuFrame.Begin();
					entry->Menu();
					entry->MenuFrame.End();
				}
			}
		}
		End();
	}
}
