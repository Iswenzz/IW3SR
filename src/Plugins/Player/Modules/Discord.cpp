#include "Discord.hpp"

namespace IW3SR::Addons
{
	constexpr float HeaderHeight = 25.0f;

	Discord::Discord() : Module("sr.player.discord", "Player", "Discord")
	{
		BarPosition = { -250, 40 };
		BarSize = { 500, 75 };

		ColorHeader = { 0.45, 0.54, 0.85, 0.6 };
		ColorBackground = { 0, 0, 0, 0.6 };

		TitleText = Text("DISCORD", FONT_SPACERANGER, -240, 43, 1.1, { 1, 1, 1, 1 });
		TitleText.SetRectAlignment(Horizontal::Center, Vertical::Top);

		RequestText = Text("", FONT_SPACERANGER, -235, 72, 0.9, { 1, 1, 1, 1 });
		RequestText.SetRectAlignment(Horizontal::Center, Vertical::Top);

		AcceptText = Text("", FONT_SPACERANGER, -110, 95, 0.8, { 0.6, 1, 0.6, 1 });
		AcceptText.SetRectAlignment(Horizontal::Center, Vertical::Top);

		DenyText = Text("", FONT_SPACERANGER, 110, 95, 0.8, { 1, 0.6, 0.6, 1 });
		DenyText.SetRectAlignment(Horizontal::Center, Vertical::Top);
		DenyText.SetAlignment(Alignment::Right, Alignment::Top);

		KeyAccept = Bind(Key_F1);
		KeyDeny = Bind(Key_F2);

		ShowOverlay = true;
	}

	void Discord::Menu()
	{
		if (!GDiscord::Available())
			ImGui::TextDisabled("Rich presence is off, or CoD4X is already providing it.");
		else if (!GDiscord::Connected())
			ImGui::TextDisabled("Waiting for Discord.");

		ImGui::Checkbox("Show Overlay", &ShowOverlay);
		ImGui::Tooltip("Draw the bar when a friend asks to join. The keys answer either way.");

		ImGui::Keybind("Accept", &KeyAccept.Input);
		ImGui::SameLine();
		ImGui::Keybind("Decline", &KeyDeny.Input);

		ImGui::DragFloat2("Bar Position", &BarPosition.x);
		ImGui::DragFloat2("Bar Size", &BarSize.x);

		ImGui::ColorEdit4("Header", &ColorHeader.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Background", &ColorBackground.x, ImGuiColorEditFlags_Float);

		TitleText.Menu("Title Options");
		RequestText.Menu("Request Options");
		AcceptText.Menu("Accept Options");
		DenyText.Menu("Decline Options");
	}

	void Discord::OnRender()
	{
		const DiscordJoinRequest* request = GDiscord::Active();
		if (!request)
			return;

		// Answering ends the request, so the frame that does it draws nothing.
		if (KeyAccept.IsPressed())
		{
			GDiscord::Respond(true);
			return;
		}
		if (KeyDeny.IsPressed())
		{
			GDiscord::Respond(false);
			return;
		}
		if (ShowOverlay)
			Draw(*request);
	}

	void Discord::Draw(const DiscordJoinRequest& request)
	{
		const int remaining = std::max(request.Expires - (cls ? cls->realtime : 0), 0) / 1000;
		const int waiting = GDiscord::Count() - 1;

		RequestText.Value = std::format("{} wants to join your game! ({})", request.Username, remaining);
		if (waiting > 0)
			RequestText.Value += std::format("   +{} waiting", waiting);

		AcceptText.Value = std::format("{} to accept", Input::GetName(KeyAccept.Input));
		DenyText.Value = std::format("{} to decline", Input::GetName(KeyDeny.Input));

		DrawBar();

		TitleText.Render();
		RequestText.Render();
		AcceptText.Render();
		DenyText.Render();
	}

	void Discord::DrawBar()
	{
		vec2 header = BarPosition;
		vec2 headerSize = { BarSize.x, HeaderHeight };
		UI::Screen.Apply(header, headerSize, Horizontal::Center, Vertical::Top);
		Draw2D::DrawQuad(vec3(header, 0), headerSize, ColorHeader);

		vec2 body = { BarPosition.x, BarPosition.y + HeaderHeight };
		vec2 bodySize = { BarSize.x, std::max(BarSize.y - HeaderHeight, 0.0f) };
		UI::Screen.Apply(body, bodySize, Horizontal::Center, Vertical::Top);
		Draw2D::DrawQuad(vec3(body, 0), bodySize, ColorBackground);
	}
}
