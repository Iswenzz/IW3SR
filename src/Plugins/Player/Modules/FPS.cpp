#include "FPS.hpp"

#include "Game/System/Timestep.hpp"

namespace IW3SR::Addons
{
	// A number redrawn three hundred times a second cannot be read, so it settles this often.
	constexpr float SettleTime = 0.2f;

	FPS::FPS() : Module("sr.player.fps", "Player", "FPS")
	{
		SwitchText = Text("0", FONT_SPACERANGER, -30, 0, 1.4, { 1, 1, 1, 1 });
		SwitchText.SetRectAlignment(Horizontal::Right, Vertical::Top);
		SwitchText.SetAlignment(Alignment::Center, Alignment::Top);

		FramesText = Text("0", FONT_SPACERANGER, -30, 20, 1.4, { 1, 1, 1, 1 });
		FramesText.SetRectAlignment(Horizontal::Right, Vertical::Top);
		FramesText.SetAlignment(Alignment::Center, Alignment::Top);

		ShowSwitch = true;
		ShowFrames = true;
	}

	void FPS::Menu()
	{
		DrawTimestep();

		ImGui::Checkbox("Show Switch FPS", &ShowSwitch);
		ImGui::Tooltip("Draw com_maxfps.");

		ImGui::Checkbox("Show Game FPS", &ShowFrames);
		ImGui::Tooltip("Draw the frame rate being reached, green while it holds the cap.");

		SwitchText.Menu("Switch Options");
		FramesText.Menu("Game Options");
	}

	void FPS::DrawTimestep()
	{
		const auto timestep = Dvar::Find("sr_timestep");
		const auto maxfps = Dvar::Find("sr_maxfps");

		if (!timestep || !maxfps)
			return;

		// The config is written from the latched value, so an edit that only lands on the current
		// one is forgotten by the next restart.
		if (ImGui::Checkbox("Enable Timestep", &timestep->current.enabled))
			timestep->latched.enabled = timestep->current.enabled;
		ImGui::Tooltip("Step movement at com_maxfps instead of at the frame rate.");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);

		// Typing a cap applies it a digit at a time, so the floor has to be a frame rate the game is
		// still usable at. Clamped, or the first digit of 144 caps the game at 1 while you finish it.
		if (ImGui::SliderInt("##sr_maxfps", &maxfps->current.integer, 60, 1000, "%d fps", ImGuiSliderFlags_AlwaysClamp))
		{
			maxfps->latched.integer = maxfps->current.integer;
		}
		ImGui::Tooltip("Frame rate cap, separate from the movement rate.");
	}

	void FPS::Measure()
	{
		Counted++;
		Elapsed += UI::DeltaTime();

		if (Elapsed < SettleTime)
			return;

		Frames = static_cast<int>(Counted / Elapsed + 0.5f);
		Counted = 0;
		Elapsed = 0;
	}

	// Green while the cap is being held, amber as it slips, red once a fifth of it is gone.
	vec4 FPS::FrameColor() const
	{
		const int cap = IW3SR::Timestep::RenderFps();
		vec4 color = { 0.45f, 1.0f, 0.45f, 1.0f };

		if (cap > 0 && Frames < cap - cap / 20)
			color = Frames < cap * 4 / 5 ? vec4{ 1.0f, 0.35f, 0.35f, 1.0f } : vec4{ 1.0f, 0.8f, 0.3f, 1.0f };

		color.w = FramesText.Color.w;
		return color;
	}

	void FPS::OnRender()
	{
		Measure();
		Switch = IW3SR::Timestep::MovementFps();

		if (ShowSwitch)
		{
			SwitchText.Value = std::to_string(Switch);
			SwitchText.Render();
		}
		if (ShowFrames)
		{
			const vec4 color = FramesText.Color;

			FramesText.Value = std::to_string(Frames);
			FramesText.Color = FrameColor();
			FramesText.Render();
			FramesText.Color = color;
		}
	}
}
