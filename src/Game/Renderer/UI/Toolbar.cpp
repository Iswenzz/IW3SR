#include "Toolbar.hpp"

#include "Core/System/Plugins.hpp"
#include "Core/System/System.hpp"

#include "Game/Renderer/GUI.hpp"
#include "Game/Renderer/Modules/Modules.hpp"
#include "Game/Renderer/Settings/Settings.hpp"

namespace IW3SR::UC
{
	Toolbar::Toolbar() : Window("Toolbar")
	{
		SetRectAlignment(HORIZONTAL_FULLSCREEN, VERTICAL_FULLSCREEN);
	}

	void Toolbar::Render()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
		SetRect(0, 0, 640, 14);

		Begin(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoMove);

		auto& GUI = GUI::Get();
		const vec2& position = RenderPosition;
		const vec2& size = RenderSize;
		const vec2 buttonSize = vec2{ 14, 14 } * UI::Get().Size;

		ImGui::Rainbow(position + vec2{ 0, size.y }, position + vec2{ size.x, size.y + 2 });
		ImGui::Button(ICON_FA_GAMEPAD, "Modules", &GUI.Modules.Open, buttonSize);
		ImGui::Tooltip("Modules");
		ImGui::SameLine();

		if (System::IsDebug())
		{
			if (ImGui::Button(ICON_FA_ROTATE_RIGHT, "Reload", nullptr, buttonSize))
				Reload();
			ImGui::Tooltip("Reload plugins");
			ImGui::SameLine();
		}
		ImGui::ButtonToggle(ICON_FA_GRIP, "Design", &GUI.UI.DesignMode, buttonSize);
		ImGui::Tooltip("Design mode");
		ImGui::SameLine();
		ImGui::Button(ICON_FA_PAINTBRUSH, "Themes", &GUI.UI.Themes.Open, buttonSize);
		ImGui::Tooltip("Themes");
		ImGui::SameLine();
		ImGui::Button(ICON_FA_KEYBOARD, "Binds", &GUI.Binds.Open, buttonSize);
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
			ImGui::Button(ICON_FA_MEMORY, "Memory", &GUI.UI.Memory.Open, buttonSize);
			ImGui::Tooltip("Memory");
			ImGui::SameLine();
		}
		ImGui::Button(ICON_FA_CIRCLE_INFO, "About", &GUI.About.Open, buttonSize);
		ImGui::Tooltip("About");
		ImGui::SameLine();
		ImGui::Button(ICON_FA_GEAR, "Settings", &GUI.Settings.Open, buttonSize);
		ImGui::Tooltip("Settings");

		End();
		ImGui::PopStyleVar(2);
	}

	void Toolbar::Reload()
	{
		if (IsReloading)
			return;
		IsReloading = true;

		IW3SR::Modules::Release();
		IW3SR::Settings::Release();

		Plugins::Shutdown();
		std::thread([this] { Compile(); }).detach();
	}

	void Toolbar::Compile()
	{
		if (std::filesystem::exists(CMAKE_BINARY_DIR))
		{
			constexpr auto command = R"(cd "{}" && cmake --build . --config Debug --target Install)";
			system(std::format(command, CMAKE_BINARY_DIR).c_str());
		}
		Plugins::Initialize();
		EventPluginGame event;
		Plugins::Dispatch(event);

		IW3SR::Modules::Initialize();
		IW3SR::Settings::Initialize();

		IsReloading = false;
	}
}
