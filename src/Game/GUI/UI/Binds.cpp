#include "Binds.hpp"

namespace IW3SR::UC
{
	Binds::Binds() : Frame("Binds") { }

	void Binds::OnRender()
	{
		Begin();
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
			ImGui::Keybind("Menu", &UI::KeyOpen.Input, false);
		End();
	}
}
