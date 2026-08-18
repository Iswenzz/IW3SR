#include "CGAZ.hpp"

namespace IW3SR::Addons
{
	CGAZ::CGAZ() : Module("sr.player.cgaz", "Player", "CGAZ")
	{
		ColorBackground = { 0.25, 0.25, 0.25, 0.7 };
		ColorPartialAccel = { 0, 1, 0, 0.7 };
		ColorFullAccel = { 0, 0.25, 0.25, 0.7 };
		ColorTurnZone = { 1, 1, 0, 0.5 };

		Y = 238;
		Height = 8;

		UseGroundZones = true;
	}

	void CGAZ::Menu()
	{
		ImGui::Checkbox("Ground Zones", &UseGroundZones);

		ImGui::DragFloat("Y Position", &Y);
		ImGui::DragFloat("Height", &Height);

		ImGui::ColorEdit4("Background", &ColorBackground.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Partial Accel", &ColorPartialAccel.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Full Accel", &ColorFullAccel.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4("Turn Zone", &ColorTurnZone.x, ImGuiColorEditFlags_Float);
	}

	void CGAZ::Compute(float wishspeed, float accel, float gravity)
	{
		g_squared = gravity * gravity;
		v_squared = glm::dot(vec2(pml.previous_velocity), vec2(pml.previous_velocity));
		vf_squared = glm::dot(vec2(pm.ps->velocity), vec2(pm.ps->velocity));
		w_speed = wishspeed;
		a = accel * wishspeed * pml.frametime;
		a_squared = a * a;

		if (!UseGroundZones || v_squared - vf_squared >= 2 * a * wishspeed - a_squared)
			v_squared = vf_squared;

		v = sqrt(v_squared);
		vf = sqrt(vf_squared);

		d_min = Min();
		d_opt = Opt();
		d_max_cos = MaxCos(d_opt);
		d_max = Max(d_max_cos);
		d_vel = atan2(pm.ps->velocity[1], pm.ps->velocity[0]);
	}

	float CGAZ::SafeAcos(float num, float den)
	{
		if (den <= 0.0f)
			return 0.0f;

		return acos(std::clamp(num / den, -1.0f, 1.0f));
	}

	float CGAZ::Min()
	{
		const float num_squared = w_speed * w_speed - v_squared + vf_squared + g_squared;
		if (num_squared < 0.0f)
			return 0.0f;

		const float num = sqrt(num_squared);
		return num >= vf ? 0 : SafeAcos(num, vf);
	}

	float CGAZ::Opt()
	{
		const float num = w_speed - a;
		return num >= vf ? 0 : SafeAcos(num, vf);
	}

	float CGAZ::MaxCos(float d_opt)
	{
		const float root = v_squared - g_squared;
		if (root < 0.0f)
			return d_opt;

		const float num = sqrt(root) - vf;
		float d_max_cos = num >= a ? 0 : SafeAcos(num, a);

		if (d_max_cos < d_opt)
			d_max_cos = d_opt;

		return d_max_cos;
	}

	float CGAZ::Max(float d_max_cos)
	{
		const float num = v_squared - vf_squared - a_squared - g_squared;
		const float den = 2 * a * vf;

		if (den <= 0.0f)
			return d_max_cos;
		if (num >= den)
			return 0;
		if (-num >= den)
			return M_PI;

		float d_max = SafeAcos(num, den);
		if (d_max < d_max_cos)
			d_max = d_max_cos;

		return d_max;
	}

	void CGAZ::DrawAngleYaw(float start, float end, float yaw, const vec4& color)
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

	void CGAZ::OnDraw2D(EventRenderer2D& event)
	{
		if (!Update())
			return;

		Compute(w_speed, accelerate, slick_gravity);

		const float yaw = atan2(w_vel[1], w_vel[0]) - d_vel;
		DrawAngleYaw(-d_min, +d_min, yaw, ColorBackground);
		DrawAngleYaw(+d_min, +d_opt, yaw, ColorPartialAccel);
		DrawAngleYaw(-d_opt, -d_min, yaw, ColorPartialAccel);
		DrawAngleYaw(+d_opt, +d_max_cos, yaw, ColorFullAccel);
		DrawAngleYaw(-d_max_cos, -d_opt, yaw, ColorFullAccel);
		DrawAngleYaw(+d_max_cos, +d_max, yaw, ColorTurnZone);
		DrawAngleYaw(-d_max, -d_max_cos, yaw, ColorTurnZone);
	}
}
