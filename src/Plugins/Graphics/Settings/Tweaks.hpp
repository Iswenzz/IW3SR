#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Graphics tweaks.
	/// </summary>
	class Tweaks : public Setting
	{
	public:
		bool DrawTweaks;
		bool DrawGlow;
		bool DrawSun;

		float TweakBrightness;
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
		/// Create the setting.
		/// </summary>
		Tweaks();
		virtual ~Tweaks() = default;

		/// <summary>
		/// Initialize setting.
		/// </summary>
		void Initialize() override;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void OnMenu() override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;
	};
}
