#include "Binds.hpp"

#include "Game/GUI/GUI.hpp"

namespace IW3SR::UC
{
	Binds::Binds() : Window("Binds") { }

	void Binds::OnRender()
	{
		Begin();
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
			ImGui::Keybind("Menu", &UI::KeyOpen.Key, false);
		End();
	}
}
