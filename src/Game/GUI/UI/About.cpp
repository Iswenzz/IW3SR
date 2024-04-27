#include "About.hpp"
#include "ImGUI/UI.hpp"

namespace IW3SR::UC
{
	About::About() : Window("About") { }

	void About::Render()
	{
		if (!Open)
			return;

		constexpr auto IW3SR = "IW3SR";
		constexpr auto markdown = R"(
IW3SR is a client modification for Call of Duty 4 powered by IzEngine.
Enhance gameplay experience and performance through a range of improvements, features such as an in game GUI with themes,
a plugin system to reload modules at runtime, interpolation of rotating platforms, Q3 (CPM) and CS movements, shaders playing offline,
single player maps, a velocity meter and more.

IW3SR (c) 2023-2024
Iswenzz Dualite Xoxor)";

		Begin();
		ImGui::PushFont(UI::Get().Themes.H1);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(IW3SR).x) * 0.5f);
		ImGui::Text(IW3SR);
		ImGui::Separator();
		ImGui::PopFont();

		ImGui::Markdown(markdown);
		End();
	}
}
