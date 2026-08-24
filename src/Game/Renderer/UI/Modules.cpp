#include "Modules.hpp"

#include "Game/System/Dvar.hpp"

namespace IW3SR::UC
{
	namespace
	{
		constexpr float SidebarRatio = 0.3f;
		constexpr float DefaultWidth = 460;
		constexpr float DefaultHeight = 300;
		constexpr float MinWidth = 400;
		constexpr float MinHeight = 260;

		constexpr auto Dot = " \xC2\xB7 ";

		// Panels that lay out to fit exactly; only the lists inside them scroll.
		constexpr ImGuiWindowFlags Fixed = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		constexpr auto SidebarColor = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
		constexpr auto PanelColor = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		constexpr auto RowColor = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		constexpr auto RowHoveredColor = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		constexpr auto RowActiveColor = ImVec4(1.00f, 1.00f, 1.00f, 0.14f);

		char Lower(char c)
		{
			return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
		}

		std::string Upper(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(),
				[](char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; });
			return text;
		}

		bool Contains(const std::string& text, const std::string& needle)
		{
			const auto it = std::search(text.begin(), text.end(), needle.begin(), needle.end(),
				[](char a, char b) { return Lower(a) == Lower(b); });
			return it != text.end();
		}

		Ref<Module> Find(const std::string& id)
		{
			const auto it = IW3SR::Modules::Entries.find(id);
			return it != IW3SR::Modules::Entries.end() ? it->second : nullptr;
		}

		float RowHeight()
		{
			return ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 1.5f;
		}

		vec2 ToggleSize()
		{
			const float height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 0.5f;
			return { height * 1.9f, height };
		}

		void BeginPanel(const std::string& id, const vec2& size, const ImVec4& background, ImGuiChildFlags flags = 0,
			ImGuiWindowFlags window = 0)
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
			ImGui::BeginChild(id.c_str(), size, flags, window);
			ImGui::PopStyleColor();
		}

		// Checkbox bound straight to a boolean dvar, so the console and the panel stay in step.
		void Setting(const char* label, const char* name, const char* tooltip)
		{
			const auto dvar = Dvar::Find(name);
			if (!dvar)
				return;

			if (ImGui::Checkbox(label, &dvar->current.enabled))
				dvar->latched.enabled = dvar->current.enabled;
			ImGui::Tooltip(std::string(tooltip) + "\n\n" + name);
		}

		// Marks the selected row with an accent bar over its left edge.
		void Accent()
		{
			const vec2 min = ImGui::GetItemRectMin();
			const vec2 max = ImGui::GetItemRectMax();
			const float width = ImGui::GetFontSize() * 0.15f;

			ImGui::GetWindowDrawList()->AddRectFilled(min, { min.x + width, max.y },
				ImGui::GetColorU32(ImGuiCol_ButtonActive));
		}
	}

	Modules::Modules() : Frame("Modules")
	{
		Reset();
		SetFlags(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
	}

	// Runs after the saved layout is restored: the panel needs room, so layouts left over from the
	// old narrow module list are dropped back to the default rect.
	void Modules::Initialize()
	{
		if (Size.x < MinWidth || Size.y < MinHeight)
			Reset();
	}

	void Modules::Reset()
	{
		SetRect(DefaultWidth * -0.5f, DefaultHeight * -0.5f, DefaultWidth, DefaultHeight);
		SetRectAlignment(Horizontal::Center, Vertical::Center);
	}

	void Modules::OnRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		Begin();
		ImGui::PopStyleVar();

		const float width = ImGui::GetContentRegionAvail().x;

		Sidebar(ImClamp(width * SidebarRatio, ImGui::GetFontSize() * 8.0f, width * 0.5f));
		ImGui::SameLine(0, 0);
		Details();

		End();
	}

	void Modules::Sidebar(float width)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		// Spacing, separator, spacing and the settings row itself; a horizontal separator adds no height.
		const float footer = RowHeight() + style.ItemSpacing.y * 5.0f;

		BeginPanel("##sidebar", { width, 0 }, SidebarColor, ImGuiChildFlags_AlwaysUseWindowPadding, Fixed);

		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS "  Search", &Filter);
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Header, RowColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, RowHoveredColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, RowActiveColor);

		BeginPanel("##list", { 0, -footer }, PanelColor);

		std::map<std::string, std::vector<Ref<Module>>> groups;
		for (const auto& [_, entry] : IW3SR::Modules::Entries)
		{
			if (Matches(entry))
				groups[entry->Group].push_back(entry);
		}
		for (auto& [group, entries] : groups)
		{
			std::sort(entries.begin(), entries.end(),
				[](const Ref<Module>& a, const Ref<Module>& b) { return a->Name < b->Name; });

			Category(group);
			for (const auto& entry : entries)
				Entry(entry);

			ImGui::Spacing();
		}
		if (groups.empty())
			ImGui::TextDisabled("No modules found.");

		ImGui::EndChild();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		const float height = RowHeight();
		const vec2 cursor = ImGui::GetCursorPos();

		if (ImGui::Selectable("##settings", ShowSettings, 0, { 0, height }))
			ShowSettings = true;
		if (ShowSettings)
			Accent();

		ImGui::SetCursorPos({ cursor.x + style.FramePadding.x, cursor.y + (height - ImGui::GetFontSize()) * 0.5f });
		ImGui::TextUnformatted(ICON_FA_GEAR "  Settings");

		ImGui::PopStyleColor(3);
		ImGui::EndChild();
	}

	void Modules::Category(const std::string& group)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().FramePadding.x);
		ImGui::TextDisabled("%s", Upper(group).c_str());
		ImGui::Spacing();
	}

	void Modules::Entry(const Ref<Module>& entry)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const vec2 toggle = ToggleSize();
		const float height = RowHeight();
		const float width = ImGui::GetContentRegionAvail().x;
		const vec2 cursor = ImGui::GetCursorPos();
		const bool selected = !ShowSettings && Selected == entry->ID;

		ImGui::PushID(entry->ID.c_str());
		ImGui::SetNextItemAllowOverlap();

		if (ImGui::Selectable("##entry", selected, ImGuiSelectableFlags_AllowOverlap, { 0, height }))
		{
			Selected = entry->ID;
			ShowSettings = false;
		}
		if (selected)
			Accent();

		ImGui::SetCursorPos(
			{ cursor.x + style.FramePadding.x * 3.0f, cursor.y + (height - ImGui::GetFontSize()) * 0.5f });
		ImGui::TextColored(ImGui::GetStyleColorVec4(entry->IsEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), "%s",
			entry->Name.c_str());

		ImGui::SetCursorPos({ cursor.x + width - toggle.x, cursor.y + (height - toggle.y) * 0.5f });
		if (ImGui::Toggle("##toggle", &entry->IsEnabled, toggle))
			entry->IsEnabled ? entry->Initialize() : entry->Release();

		ImGui::SetCursorPos({ cursor.x, cursor.y + height + style.ItemSpacing.y * 0.5f });
		ImGui::PopID();
	}

	void Modules::Details()
	{
		BeginPanel("##details", { 0, 0 }, PanelColor, ImGuiChildFlags_AlwaysUseWindowPadding, Fixed);

		const auto entry = ShowSettings ? nullptr : Find(Selected);

		if (ShowSettings)
		{
			Settings();
		}
		else if (entry)
		{
			ImGui::PushID(entry->ID.c_str());
			Header(entry->Name, entry->Group + Dot + entry->ID);

			BeginPanel("##content", { 0, 0 }, PanelColor);
			const float cursor = ImGui::GetCursorPosY();
			entry->Menu();

			if (ImGui::GetCursorPosY() == cursor)
				ImGui::TextDisabled("This module has no options.");
			ImGui::EndChild();
			ImGui::PopID();
		}
		else
		{
			Placeholder();
		}
		ImGui::EndChild();
	}

	void Modules::Header(const std::string& title, const std::string& subtitle)
	{
		ImGui::PushFont(ImGui::H3);
		ImGui::TextUnformatted(title.c_str());
		ImGui::PopFont();
		ImGui::TextDisabled("%s", subtitle.c_str());

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}

	void Modules::Settings()
	{
		Header("Settings", std::string("Interface") + Dot + "Input");

		BeginPanel("##settings", { 0, 0 }, PanelColor);
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Keybind("Menu", &UI::KeyOpen.Input, false);
			ImGui::Checkbox("Design Mode", &UI::DesignMode);

			Setting("Raw Input", "sr_rawinput",
				"Reads the mouse through raw input instead of the Windows pointer.\n"
				"Drops pointer acceleration and the movement the old path lost at the\n"
				"edges of the screen, and stays smooth and stable on the frame rate at\n"
				"high polling rates, where 1000 Hz and above used to stutter and flicker.");
		}
		ImGui::EndChild();
	}

	void Modules::Placeholder()
	{
		constexpr auto text = "Select a module";

		const ImGuiStyle& style = ImGui::GetStyle();
		const vec2 cursor = ImGui::GetCursorPos();
		const vec2 avail = ImGui::GetContentRegionAvail();
		const vec2 icon = ImGui::CalcTextSize(ICON_FA_CUBES);
		const vec2 size = ImGui::CalcTextSize(text);
		const float height = icon.y + style.ItemSpacing.y + size.y;

		ImGui::SetCursorPos({ cursor.x + (avail.x - icon.x) * 0.5f, cursor.y + (avail.y - height) * 0.5f });
		ImGui::TextDisabled("%s", ICON_FA_CUBES);

		ImGui::SetCursorPosX(cursor.x + (avail.x - size.x) * 0.5f);
		ImGui::TextDisabled("%s", text);
	}

	bool Modules::Matches(const Ref<Module>& entry) const
	{
		if (Filter.empty())
			return true;
		return Contains(entry->Name, Filter) || Contains(entry->Group, Filter);
	}
}
