#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Display type.
	/// </summary>
	enum class Display
	{
		Player,
		Mode,
		FPS,
		Velocity,
		Map,
		Time,
		Health
	};

	/// <summary>
	/// Kinetic movement HUD.
	/// </summary>
	class KMOV : public Module
	{
	public:
		float JumpPower = 1;
		float JumpMax = 10;
		float CameraPower = 1;
		float FireMagnitude = 0.5;
		float FireSpeed = 20;

		Text TextLT;
		Text TextLB;
		Text TextRT;
		Text TextRB;

		Display DisplayLT = Display::Player;
		Display DisplayLB = Display::Mode;
		Display DisplayRT = Display::FPS;
		Display DisplayRB = Display::Velocity;

		/// <summary>
		/// Create the module.
		/// </summary>
		KMOV();
		virtual ~KMOV() = default;

		/// <summary>
		/// Initialize module.
		/// </summary>
		void Initialize() override;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void Menu() override;

		/// <summary>
		/// Client spawn.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnSpawn(EventClientSpawn& event) override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;

	private:
		vec2 OriginalPositionLT;
		vec2 OriginalPositionLB;
		vec2 OriginalPositionRT;
		vec2 OriginalPositionRB;
		vec2 CurrentOffset;

		int StartTime = 0;
		float LandingOrigin = 0;
		float JumpOrigin = 0;
		vec3 AnglesDelta;
		float FireDuration = 0;

		bool IsAttacking = false;
		bool IsShaking = false;
		bool IsBouncing = false;

		/// <summary>
		/// Compute values.
		/// </summary>
		void Compute();

		/// <summary>
		/// Angles animation.
		/// </summary>
		/// <returns></returns>
		vec2 Angles();

		/// <summary>
		/// Jump animation.
		/// </summary>
		/// <returns></returns>
		float Jump();

		/// <summary>
		/// Fire shaking animation.
		/// </summary>
		/// <returns></returns>
		vec2 Fire();

		/// <summary>
		/// Get the value for display.
		/// </summary>
		/// <param name="display">The display.</param>
		/// <returns></returns>
		const std::string ComputeValue(Display display);

		SERIALIZE_POLY(KMOV, Module, TextLT, TextLB, TextRT, TextRB, DisplayLT, DisplayLB, DisplayRT, DisplayRB)
	};
}
