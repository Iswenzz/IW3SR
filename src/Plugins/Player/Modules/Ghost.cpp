#include "Ghost.hpp"

namespace IW3SR::Addons
{
	namespace
	{
		// Above the ghost's feet, clear of its head.
		constexpr float LabelHeight = 78.0f;

		std::string FormatTime(int ms)
		{
			return std::format("{}:{:02}.{:03}", ms / 60000, ms / 1000 % 60, ms % 1000);
		}

		// The name without its ^ color codes, which only the engine's text draws.
		std::string PlainName(const std::string& name)
		{
			std::string plain;
			for (size_t i = 0; i < name.size(); i++)
			{
				if (name[i] == '^' && i + 1 < name.size() && std::isdigit(static_cast<unsigned char>(name[i + 1])))
					i++;
				else
					plain.push_back(name[i]);
			}
			return plain;
		}
	}

	Ghost::Ghost() : Module("sr.player.ghost", "Player", "Ghost")
	{
		DeltaText = Text("+0.00", FONT_SPACERANGER, 0, 60, 1.4, { 1, 1, 1, 1 });
		DeltaText.SetRectAlignment(Horizontal::Center, Vertical::Center);
		DeltaText.SetAlignment(Alignment::Center, Alignment::Top);

		ShowDelta = true;
		ShowLabel = true;
		HideDistance = 96.0f;
		Opacity = 0.5f;
	}

	void Ghost::Menu()
	{
		if (ImGui::BeginSection("General"))
		{
			ImGui::Property("Show Delta");
			ImGui::Switch("##delta", &ShowDelta);
			ImGui::Tooltip("Draw how far ahead of the ghost the run is, or behind it.");

			ImGui::Property("Show Label");
			ImGui::Switch("##label", &ShowLabel);
			ImGui::Tooltip("Draw the record's holder and time over the ghost.");

			ImGui::Property("Hide Distance");
			ImGui::DragFloat("##hide", &HideDistance, 1.0f, 0.0f, 500.0f);
			ImGui::Tooltip("The ghost is not drawn closer to the view than this.");

			ImGui::Property("Opacity");
			ImGui::SliderFloat("##opacity", &Opacity, 0.1f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Tooltip("How solid the ghost is drawn. At 1 it is drawn like any other player.");
			ImGui::EndSection();
		}
		DeltaText.Menu("Delta Options");
	}

	void Ghost::OnRender()
	{
		ServerGhost::HideDistance = HideDistance;
		GGhost::Opacity = Opacity;

		const ServerGhostState* ghost = ServerGhost::Get();
		if (!ghost)
			return;

		if (ShowDelta && ghost->HasDelta)
			DrawDelta(*ghost);
		if (ShowLabel)
			DrawLabel(*ghost);
	}

	// Green ahead of the ghost, red behind it.
	void Ghost::DrawDelta(const ServerGhostState& ghost)
	{
		const vec4 color = DeltaText.Color;
		const int ms = std::abs(ghost.Delta);

		DeltaText.Value = std::format("{}{}.{:02}", ghost.Delta < 0 ? '-' : '+', ms / 1000, ms / 10 % 100);
		DeltaText.Color = ghost.Delta < 0 ? vec4{ 0.45f, 1.0f, 0.45f, color.w } : vec4{ 1.0f, 0.35f, 0.35f, color.w };
		DeltaText.Render();
		DeltaText.Color = color;
	}

	void Ghost::DrawLabel(const ServerGhostState& ghost)
	{
		vec3 origin{};
		if (!ServerGhost::Origin(origin))
			return;

		const vec2 screen = GMath::WorldToScreen(origin + vec3(0.0f, 0.0f, LabelHeight));
		if (!glm::length2(screen))
			return;

		std::string label = PlainName(ghost.Name);
		if (ghost.RecordTime > 0)
			label += "  " + FormatTime(ghost.RecordTime);

		const ImVec2 size = ImGui::CalcTextSize(label.c_str());
		const ImVec2 at = { screen.x - size.x / 2, screen.y - size.y };
		auto draw = ImGui::GetBackgroundDrawList();
		draw->AddText({ at.x + 1, at.y + 1 }, IM_COL32(0, 0, 0, 200), label.c_str());
		draw->AddText(at, IM_COL32(255, 255, 255, 230), label.c_str());
	}
}
