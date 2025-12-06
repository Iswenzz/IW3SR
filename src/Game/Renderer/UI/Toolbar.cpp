#include "Toolbar.hpp"

#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/Renderer/Modules/Settings.hpp"

namespace IW3SR::UC
{
	Toolbar::Toolbar() : Frame("Toolbar")
	{
		Open = true;
		SetRectAlignment(Horizontal::Fullscreen, Vertical::Fullscreen);
		SetFlags(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
	}

	void Toolbar::OnRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
		SetRect(0, 0, 640, 16);

		Begin();

		const vec2 position = RenderPosition;
		const vec2 size = RenderSize;
		const vec2 buttonSize = { size.y, size.y };

		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		draw->AddRectFilled(position, position + size, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg)));

		ImGui::Rainbow(position + vec2{ 0, size.y }, position + vec2{ size.x, size.y + 2 });
		ImGui::Button(ICON_FA_GAMEPAD, "Modules", &UI::Frames["Modules"]->Open, buttonSize);
		ImGui::Tooltip("Modules");
		ImGui::SameLine();

		if (System::IsDebug())
		{
			if (ImGui::Button(ICON_FA_ROTATE_RIGHT, "Reload", nullptr, buttonSize))
				Reload();

			ImGui::Tooltip("Reload plugins");
			ImGui::SameLine();
		}
		ImGui::ButtonToggle(ICON_FA_GRIP, "Design", &UI::DesignMode, buttonSize);
		ImGui::Tooltip("Design mode");
		ImGui::SameLine();
		ImGui::Button(ICON_FA_PAINTBRUSH, "Themes", &UI::Frames["Themes"]->Open, buttonSize);
		ImGui::Tooltip("Themes");
		ImGui::SameLine();
		ImGui::Button(ICON_FA_KEYBOARD, "Binds", &UI::Frames["Binds"]->Open, buttonSize);
		ImGui::Tooltip("Binds");
		ImGui::SameLine();

		if (System::IsDebug())
		{
			ImGui::Button(ICON_FA_TERMINAL, "Debug", &IsDebug, buttonSize);
			ImGui::Tooltip("Debug");
			ImGui::SameLine();

			if (IsDebug)
			{
				ImGui::ShowDebugLogWindow(&IsDebug);
				ImGui::ShowStackToolWindow(&IsDebug);
			}
			ImGui::Button(ICON_FA_MEMORY, "Memory", &UI::Frames["Memory"]->Open, buttonSize);
			ImGui::Tooltip("Memory");
			ImGui::SameLine();
		}
		ImGui::Button(ICON_FA_CIRCLE_INFO, "About", &UI::Frames["About"]->Open, buttonSize);
		ImGui::Tooltip("About");
		ImGui::SameLine();
		ImGui::Button(ICON_FA_GEAR, "Settings", &UI::Frames["Settings"]->Open, buttonSize);
		ImGui::Tooltip("Settings");

		End();
		ImGui::PopStyleVar(2);
	}

	void Toolbar::Reload()
	{
		if (IsReloading)
			return;

		IsReloading = true;

		Plugins::Shutdown();
		Plugins::Free();
		IW3SR::Modules::Serialize();
		IW3SR::Settings::Serialize();

		std::thread([this] { Compile(); }).detach();
	}

	void Toolbar::Compile()
	{
		if (!std::filesystem::exists(CMAKE_BINARY_DIR))
			return;

		system("cd /d \"" CMAKE_BINARY_DIR "\" && cmake --build . --config Debug --target Install");

		Actions::Add(
			[this]()
			{
				IW3SR::Settings::Deserialize();
				IW3SR::Modules::Deserialize();
				Plugins::Load();
				Plugins::Initialize();
				IsReloading = false;
			});
	}
}
