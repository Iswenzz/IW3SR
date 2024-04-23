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
A Call of Duty 4 client modification.

[https://github.com/Iswenzz/IW3SR](https://github.com/Iswenzz/IW3SR)
IW3SR (ent) 2023-2024)";

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
