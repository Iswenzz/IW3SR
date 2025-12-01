#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
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

		Tweaks();
		virtual ~Tweaks() = default;

		void Menu() override;
		void OnRender() override;
	};
}
