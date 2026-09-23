#pragma once
#include "Graphics/Base.hpp"

namespace IW3SR::Addons
{
	class Tweaks : public Module
	{
	public:
		bool DrawTweaks;
		bool DrawGlow;
		bool DrawSun;
		bool SunOverride;

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

		Tweaks();
		virtual ~Tweaks() = default;

		void Release() override;
		void Menu() override;
		void OnRender() override;

	private:
		void ApplySun();

		SERIALIZE_POLY(Tweaks, Module, DrawTweaks, DrawGlow, DrawSun, SunOverride, TweakBrightness, TweakContrast,
			TweakDesaturation, TweakLightTint, TweakDarkTint, GlowRadius, GlowBloomDesaturation, GlowBloomIntensity,
			GlowBloomCutoff, SunIntensity, SunColor, SunDirection)
	};
}
