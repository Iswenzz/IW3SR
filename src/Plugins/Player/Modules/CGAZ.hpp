#pragma once
// https://github.com/Jelvan1/cgame_proxymod/blob/master/src/cg_cgaz.c
// https://github.com/xoxor4d/iw3xo-dev/blob/master/src/components/modules/cgaz.cpp
#include "Base.hpp"

namespace IW3SR::Addons
{
	class CGAZ : public Module
	{
	public:
		float Y;
		float Height;

		vec4 ColorBackground;
		vec4 ColorPartialAccel;
		vec4 ColorFullAccel;
		vec4 ColorTurnZone;
		bool UseGroundZones;

		CGAZ();
		virtual ~CGAZ() = default;

		void Menu() override;
		void DrawAngleYaw(float start, float end, float yaw, const vec4& color);
		void OnDraw2D(EventRenderer2D& event) override;

	private:
		pmove_t pm = {};
		pml_t pml = {};
		vec2 w_vel;
		float w_speed = 0;
		float v = 0;
		float v_squared = 0;
		float vf = 0;
		float vf_squared = 0;
		float a = 0;
		float a_squared = 0;
		float g_squared = 0;
		float d_min = 0;
		float d_opt = 0;
		float d_max_cos = 0;
		float d_max = 0;
		float d_vel = 0;

		void PmoveSingle();
		void WalkMove();
		void AirMove();
		void Compute(float wishspeed, float accel, float gravity);
		void Accelerate(float wishspeed, float accel);
		void SlickAccelerate(float wishspeed, float accel);

		float CmdScale(playerState_s* ps, usercmd_s* cmd);
		float CmdScaleWalk(usercmd_s* cmd);
		float CmdScaleForStance();
		float DamageScaleWalk(int damageTimer);

		float GetViewHeightLerpTime(int iTarget, int bDown);
		float GetViewHeightLerp(int fromHeight, int toHeight);

		float Min();
		float Opt();
		float MaxCos(float opt);
		float Max(float maxCos);

		SERIALIZE_POLY(CGAZ, Module, ColorBackground, ColorPartialAccel, ColorFullAccel, ColorTurnZone, UseGroundZones)
	};
}
