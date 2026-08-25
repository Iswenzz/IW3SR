#include "Demo.hpp"

#include "Game/System/Assets.hpp"
#include "Game/System/Capture.hpp"
#include "Game/System/Dvar.hpp"

namespace IW3SR::UC
{
	Demo::Demo() : Frame("Demo")
	{
		SetRectAlignment(Horizontal::Center, Vertical::Center);
		SetFlags(ImGuiWindowFlags_NoCollapse);
	}

	void Demo::OnRender()
	{
		SetRect(-210, -140, 420, 280);

		Begin();
		if (ImGui::BeginTabBar("Demo"))
		{
			if (ImGui::BeginTabItem("Capture"))
			{
				DrawCaptureTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Assets"))
			{
				DrawAssetsTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		End();
	}

	void Demo::DrawCaptureTab()
	{
		static char output[256] = "capture";

		const auto fps = Dvar::Find("sr_capture_fps");
		const auto quality = Dvar::Find("sr_capture_quality");

		ImGui::InputText("Output", output, sizeof(output));

		if (fps)
			ImGui::SliderInt("Frame rate", &fps->current.integer, 1, 240);
		if (quality)
			ImGui::SliderInt("Quality", &quality->current.integer, 0, 51);

		ImGui::Spacing();

		if (Capture::Recording)
		{
			if (ImGui::Button(ICON_FA_STOP " Stop", ImVec2(-1, 0)))
				Capture::Stop();
		}
		else if (ImGui::Button(ICON_FA_VIDEO " Record", ImVec2(-1, 0)))
		{
			Capture::Start(output);
		}
		ImGui::Spacing();
		ImGui::TextDisabled("Encoding runs through ffmpeg, audio is not captured.");
	}

	void Demo::DrawAssetsTab()
	{
		if (Assets::Skipped.empty())
		{
			ImGui::TextDisabled("Every fastfile this session asked for was installed.");
			return;
		}
		ImGui::TextWrapped("These fastfiles are not installed and were skipped. What they provide falls back to "
						   "the default assets.");
		ImGui::Spacing();

		for (const auto& name : Assets::Skipped)
			ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "%s", name.c_str());
	}
}
