#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Modify general graphics.
	/// </summary>
	class General : public Setting
	{
	public:
		bool DrawSpecular;
		bool DrawFog;
		bool DrawDecals;
		bool WaterAnimation;
		bool DrawTweaks;
		bool DrawGlow;
		bool DrawSun;

		float SunIntensity;
		float SpecularColorScale;
		float TweakBrightness;
		float TweakDesaturation;
		float GlowRadius;
		float GlowBloomDesaturation;
		float GlowBloomIntensity;
		float GlowBloomCutoff;

		vec4 SunColor;
		vec3 SunDirection;
		vec3 TweakLightTint;
		vec3 TweakDarkTint;

		/// <summary>
		/// Create the setting.
		/// </summary>
		General();
		virtual ~General() = default;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void OnMenu() override;

		SERIALIZE_POLY(General, Setting, DrawSpecular, DrawFog, DrawDecals, DrawTweaks, DrawGlow, DrawSun, SunIntensity,
			SpecularColorScale, TweakBrightness, TweakDesaturation, GlowRadius, GlowBloomDesaturation,
			GlowBloomIntensity, GlowBloomCutoff, SunColor, SunDirection, TweakLightTint, TweakDarkTint)
	};
}
