#include "Snap.hpp"

#define QUADRANT (static_cast<float>(M_PI) * 0.5f)
#define MAX_ACCEL 50.0f

namespace IW3SR::Addons
{
	Snap::Snap() : Module("sr.player.snap", "Player", "Snap")
	{
		ColorPrimary = { 0, 1, 1, 0.4 };
		ColorAlternate = { 0.05, 0.05, 0.05, 0.12 };
		ColorActive = { 0, 0.9, 0.9, 0.4 };

		// Same line as the CGAZ bar
		Y = 238;
		Height = 2;

		UseActiveZone = true;
	}

	void Snap::Menu()
	{
		ImGui::Checkbox("Active Zone", &UseActiveZone);

		ImGui::DragFloat("Y Position", &Y);
		ImGui::DragFloat("Height", &Height);

		ImGui::ColorEdit4("Primary", &ColorPrimary.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Alternate", &ColorAlternate.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Active", &ColorActive.x, ImGuiColorEditFlags_Float);
	}

	// Pmove snaps the velocity to whole units every frame, so what a wish direction really adds is
	// its acceleration components rounded to integers. Directions rounding to the same pair form a
	// zone, bordered where a component crosses a .5 boundary. The pattern repeats every quadrant
	// because both axes snap the same way.
	void Snap::Build(float accel)
	{
		const int steps = static_cast<int>(accel + 0.5f);
		count = 0;

		for (int i = 0; i < steps && count + 2 <= MAX_BORDERS; i++)
		{
			const float cosine = (i + 0.5f) / accel;
			if (cosine >= 1.0f)
				continue;

			// Every border of the x component mirrors to one of the y component
			const float angle = std::acos(cosine);
			borders[count++] = angle;
			borders[count++] = QUADRANT - angle;
		}
		std::sort(borders.begin(), borders.begin() + count);
	}

	void Snap::DrawZones(float yaw)
	{
		for (int quadrant = 0; quadrant < 4; quadrant++)
		{
			for (int i = 0; i < count; i++)
			{
				const float base = quadrant * QUADRANT;
				const float start = borders[i] + base;
				const float end = (i + 1 < count ? borders[i + 1] : borders[0] + QUADRANT) + base;

				vec4 color = i & 1 ? ColorAlternate : ColorPrimary;

				// Zones stay narrower than a quadrant, so the wish direction is inside this one
				// when it sits past the start border and short of the end border
				if (UseActiveZone && Math::AngleNormalizePI(yaw - start) >= 0.0f
					&& Math::AngleNormalizePI(yaw - end) < 0.0f)
					color = ColorActive;

				DrawAngleYaw(start, end, yaw, color);
			}
		}
	}

	void Snap::DrawAngleYaw(float start, float end, float yaw, const vec4& color)
	{
		const auto& scale = UI::Screen.VirtualToFull;
		const vec3 range = Math::AnglesToRange(start, end, yaw, cgs->refdef.tanHalfFovX);

		if (!range.z)
		{
			GDraw2D::Rect(rgp->whiteMaterial, vec2{ range.x, Y } * scale, vec2{ range.y - range.x, Height } * scale,
				color);
			return;
		}
		GDraw2D::Rect(rgp->whiteMaterial, vec2{ 0, Y } * scale, vec2{ range.x, Height } * scale, color);
		GDraw2D::Rect(rgp->whiteMaterial, vec2{ range.y, Y } * scale, vec2{ SCREEN_WIDTH - range.y, Height } * scale,
			color);
	}

	void Snap::OnDraw2D(EventRenderer2D& event)
	{
		if (!Update())
			return;

		// The zone count follows the pmove step, and the measured frame time swings by a whole
		// millisecond, which rebuilds a different grid every frame. Model the step from the frame
		// rate the player is capped at instead so the zones hold still.
		const int maxfps = Dvar::Get<int>("com_maxfps");
		const float msec = maxfps > 0 ? std::floor(1000.0f / maxfps) : static_cast<float>(cgs->frametime);

		// Beyond the cap the zones are thinner than a pixel
		const float accel = std::min(accelerate * w_speed * (std::max(msec, 1.0f) / 1000.0f), MAX_ACCEL);
		if (accel <= 0.0f)
			return;

		Build(accel);
		if (!count)
			return;

		DrawZones(atan2(w_vel[1], w_vel[0]));
	}
}
