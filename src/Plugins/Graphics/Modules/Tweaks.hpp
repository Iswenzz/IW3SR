#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Graphics tweaks.
	/// </summary>
	class Tweaks : public Module
	{
	public:
		bool DrawTweaks;
		bool DrawGlow;
		bool DrawSun;

		float TweakBrightness;
		float TweakContrast;
		float TweakDesaturation;
		vec3 TweakLightTint;
		vec3 TweakDarkTint;

		float GlowRadius;
		float GlowBloomDesaturation;
		float GlowBloomIntensity;
		float GlowBloomCutoff;

		float SunIntensity;
		vec4 SunColor;
		vec3 SunDirection;

		/// <summary>
		/// Create the module.
		/// </summary>
		Tweaks();
		virtual ~Tweaks() = default;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void Menu() override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
