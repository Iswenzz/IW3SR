#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// CGAZ HUD.
	/// https://github.com/Jelvan1/cgame_proxymod/blob/master/src/cg_cgaz.c
	/// https://github.com/xoxor4d/iw3xo-dev/blob/master/src/components/modules/cgaz.cpp
	/// </summary>
	class CGAZ : public Module
	{
	public:
		float Y;
		float Height;

		Material* White;
		vec4 ColorBackground;
		vec4 ColorPartialAccel;
		vec4 ColorFullAccel;
		vec4 ColorTurnZone;

		bool UseGroundZones;

		/// <summary>
		/// Create the module.
		/// </summary>
		CGAZ();
		virtual ~CGAZ() = default;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void Menu() override;

		/// <summary>
		/// Draw a section of a circle with a specified yaw angle.
		/// </summary>
		/// <param name="start">Starting angle in radians.</param>
		/// <param name="end">Ending angle in radians.</param>
		/// <param name="yaw">Yaw angle in radians.</param>
		/// <param name="color">The color.</param>
		void DrawAngleYaw(float start, float end, float yaw, const vec4& color);

		/// <summary>
		/// Draw 2D.
		/// </summary>
		/// <param name="event">The event.</param>
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

		/// <summary>
		/// Player movement.
		/// </summary>
		void PmoveSingle();

		/// <summary>
		/// Walk move.
		/// </summary>
		void WalkMove();

		/// <summary>
		/// Air move.
		/// </summary>
		void AirMove();

		/// <summary>
		/// Compute cgaz.
		/// </summary>
		/// <param name="wishspeed">The wish speed.</param>
		/// <param name="accel">The accel.</param>
		/// <param name="gravity">The gravity.</param>
		void Compute(float wishspeed, float accel, float gravity);

		/// <summary>
		/// Accelerate.
		/// </summary>
		/// <param name="wishspeed">The wish speed.</param>
		/// <param name="accel">The accel.</param>
		void Accelerate(float wishspeed, float accel);

		/// <summary>
		/// Slick accelerate.
		/// </summary>
		/// <param name="wishspeed">The wish speed.</param>
		/// <param name="accel">The accel.</param>
		void SlickAccelerate(float wishspeed, float accel);

		/// <summary>
		/// Command scale.
		/// </summary>
		/// <param name="ps">The player state.</param>
		/// <param name="cmd">The command.</param>
		/// <returns></returns>
		float CmdScale(playerState_s* ps, usercmd_s* cmd);

		/// <summary>
		/// Command scale walk.
		/// </summary>
		/// <param name="cmd">The command.</param>
		/// <returns></returns>
		float CmdScaleWalk(usercmd_s* cmd);

		/// <summary>
		/// Command scale for stance.
		/// </summary>
		/// <returns></returns>
		float CmdScaleForStance();

		/// <summary>
		/// Damage scale walk.
		/// </summary>
		/// <param name="damageTimer">The damage timer.</param>
		/// <returns></returns>
		float DamageScaleWalk(int damageTimer);

		/// <summary>
		/// Get view height lerp time.
		/// </summary>
		/// <param name="iTarget">The target.</param>
		/// <param name="bDown">Target down.</param>
		/// <returns></returns>
		float GetViewHeightLerpTime(int iTarget, int bDown);

		/// <summary>
		/// Get view height lerp.
		/// </summary>
		/// <param name="fromHeight">From height.</param>
		/// <param name="toHeight">To height.</param>
		/// <returns></returns>
		float GetViewHeightLerp(int fromHeight, int toHeight);

		/// <summary>
		/// Min value.
		/// </summary>
		/// <returns></returns>
		float Min();

		/// <summary>
		/// Opt value.
		/// </summary>
		/// <returns></returns>
		float Opt();

		/// <summary>
		/// Max cos value.
		/// </summary>
		/// <param name="opt">Opt value.</param>
		/// <returns></returns>
		float MaxCos(float opt);

		/// <summary>
		/// Max value.
		/// </summary>
		/// <param name="maxCos">Max cos value.</param>
		/// <returns></returns>
		float Max(float maxCos);

		SERIALIZE_POLY(CGAZ, Module, ColorBackground, ColorPartialAccel, ColorFullAccel, ColorTurnZone, UseGroundZones)
	};
}
