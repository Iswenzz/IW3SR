#include "About.hpp"

namespace IW3SR::UC
{
	About::About() : Frame("About")
	{
		SetRect(-200, -95, 400, 200);
		SetRectAlignment(Horizontal::Center, Vertical::Center);
	}

	void About::OnRender()
	{
		constexpr auto IW3SR = "IW3SR";
		constexpr auto markdown = R"(
IW3SR is a client modification for Call of Duty 4 powered by IzEngine.
Enhance gameplay experience and performance through a range of improvements, features such as an in game GUI, a plugin system to reload modules at runtime, interpolation of rotating platforms, Q3 (CPM) and CS movements, CGAZ hud, shaders playing offline, a velocity meter and more.

IW3SR (c) 2026
Iswenzz Dualite xoxor4d)";

		Begin();
		ImGui::PushFont(ImGui::H1);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(IW3SR).x) * 0.5f);
		ImGui::Text(IW3SR);
		ImGui::Separator();
		ImGui::PopFont();

		ImGui::Markdown(markdown);
		ImGui::Text("Version " APPLICATION_VERSION);
		End();
	}
}
