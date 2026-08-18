#pragma once
// https://github.com/Jelvan1/cgame_proxymod/blob/master/src/cg_snap.c
#include "Player/Base.hpp"
#include "Player/Modules/PmoveHud.hpp"

namespace IW3SR::Addons
{
	class Snap : public Module, public PmoveHud
	{
	public:
		float Y;
		float Height;

		vec4 ColorPrimary;
		vec4 ColorAlternate;
		vec4 ColorActive;

		bool UseActiveZone;

		Snap();
		virtual ~Snap() = default;

		void Menu() override;
		void DrawAngleYaw(float start, float end, float yaw, const vec4& color);
		void OnDraw2D(EventRenderer2D& event) override;

	private:
		// Two borders per integer step of the accel, and the accel is capped at 50
		static constexpr int MAX_BORDERS = 128;

		std::array<float, MAX_BORDERS> borders = {};
		int count = 0;

		void Build(float accel);
		void DrawZones(float yaw);

		SERIALIZE_POLY(Snap, Module, ColorPrimary, ColorAlternate, ColorActive, UseActiveZone)
	};
}
