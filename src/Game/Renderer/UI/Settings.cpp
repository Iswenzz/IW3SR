#include "Settings.hpp"

namespace IW3SR::UC
{
	Settings::Settings() : Frame("Settings")
	{
		SetRect(-170, 20, 150, 100);
		SetRectAlignment(Horizontal::Right, Vertical::Top);
	}

	void Settings::OnRender()
	{
		Begin();
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
			ImGui::Keybind("Menu", &UI::KeyOpen.Input, false);
		End();
	}
}
