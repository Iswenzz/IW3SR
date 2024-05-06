#include "Binds.hpp"

#include "Game/GUI/GUI.hpp"

namespace IW3SR::UC
{
	Binds::Binds() : Window("Binds") { }

	void Binds::Render()
	{
		if (!Open)
			return;

		Begin();
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
			ImGui::Keybind("Menu", &GUI::Get().KeyOpen.Key, false);
		End();
	}
}
