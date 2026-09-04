#include "Tweaks.hpp"

namespace IW3SR::Addons
{
	Tweaks::Tweaks() : Module("sr.graphics.tweaks", "Graphics", "Tweaks")
	{
		DrawTweaks = false;
		DrawGlow = false;
		DrawSun = true;

		TweakBrightness = 0;
		TweakContrast = 1;
		TweakDesaturation = 0;
		TweakLightTint = { 1, 1, 1 };
		TweakDarkTint = { 1, 1, 1 };

		GlowRadius = 5;
		GlowBloomDesaturation = 0;
		GlowBloomIntensity = 1;
		GlowBloomCutoff = 0.5;

		SunIntensity = 1;
		SunColor = { 1, 1, 1, 1 };
		SunDirection = { 270, 90, 0 };
	}

	void Tweaks::Menu()
	{
		if (ImGui::CollapsingHeader("Tweaks"))
		{
			ImGui::Checkbox("Enabled", &DrawTweaks);
			ImGui::SliderFloat("Brightness", &TweakBrightness, -1, 1);
			ImGui::SliderFloat("Contrast", &TweakContrast, 0, 4);
			ImGui::SliderFloat("Desaturation", &TweakDesaturation, 0, 1);
			ImGui::ColorEdit3("Light Tint", &TweakLightTint.x, ImGuiColorEditFlags_Float);
			ImGui::ColorEdit3("Dark Tint", &TweakDarkTint.x, ImGuiColorEditFlags_Float);
		}
		if (ImGui::CollapsingHeader("Glow"))
		{
			ImGui::Checkbox("Enabled", &DrawGlow);
			ImGui::SliderFloat("Radius", &GlowRadius, 0, 32);
			ImGui::SliderFloat("Bloom Desaturation", &GlowBloomDesaturation, 0, 1);
			ImGui::SliderFloat("Bloom Intensity", &GlowBloomIntensity, 0, 20);
			ImGui::SliderFloat("Bloom Cut-off", &GlowBloomCutoff, 0, 1);
		}
		if (ImGui::CollapsingHeader("Sun"))
		{
			ImGui::Checkbox("Enabled", &DrawSun);
			ImGui::SliderFloat("Intensity", &SunIntensity, 0, 4);
			ImGui::ColorEdit4("Color", &SunColor.x, ImGuiColorEditFlags_Float);
			ImGui::SliderFloat3("Direction", &SunDirection.x, -360, 360);
		}
	}

	void Tweaks::OnRender()
	{
		static const auto r_filmTweakEnable = Dvar::Find("r_filmTweakEnable");
		static const auto r_filmUseTweaks = Dvar::Find("r_filmUseTweaks");
		static const auto r_glow = Dvar::Find("r_glow");
		static const auto r_glowUseTweaks = Dvar::Find("r_glowUseTweaks");
		static const auto r_drawSun = Dvar::Find("r_drawSun");
		static const auto r_filmTweakBrightness = Dvar::Find("r_filmTweakBrightness");
		static const auto r_filmTweakContrast = Dvar::Find("r_filmTweakContrast");
		static const auto r_filmTweakDesaturation = Dvar::Find("r_filmTweakDesaturation");
		static const auto r_filmTweakLightTint = Dvar::Find("r_filmTweakLightTint");
		static const auto r_filmTweakDarkTint = Dvar::Find("r_filmTweakDarkTint");
		static const auto r_glowTweakRadius0 = Dvar::Find("r_glowTweakRadius0");
		static const auto r_glowTweakBloomDesaturation = Dvar::Find("r_glowTweakBloomDesaturation");
		static const auto r_glowTweakBloomIntensity0 = Dvar::Find("r_glowTweakBloomIntensity0");
		static const auto r_glowTweakBloomCutoff = Dvar::Find("r_glowTweakBloomCutoff");
		static const auto r_envMapSunIntensity = Dvar::Find("r_envMapSunIntensity");
		static const auto r_lightTweakSunColor = Dvar::Find("r_lightTweakSunColor");
		static const auto r_lightTweakSunDirection = Dvar::Find("r_lightTweakSunDirection");

		r_filmTweakEnable->current.enabled = DrawTweaks;
		r_filmUseTweaks->current.enabled = DrawTweaks;
		r_glow->current.enabled = DrawGlow;
		r_glowUseTweaks->current.enabled = DrawGlow;
		r_drawSun->current.enabled = DrawSun;
		r_filmTweakBrightness->current.value = TweakBrightness;
		r_filmTweakContrast->current.value = TweakContrast;
		r_filmTweakDesaturation->current.value = TweakDesaturation;
		r_filmTweakLightTint->current.vector = vec4(TweakLightTint, 1);
		r_filmTweakDarkTint->current.vector = vec4(TweakDarkTint, 1);
		r_glowTweakRadius0->current.value = GlowRadius;
		r_glowTweakBloomDesaturation->current.value = GlowBloomDesaturation;
		r_glowTweakBloomIntensity0->current.value = GlowBloomIntensity;
		r_glowTweakBloomCutoff->current.value = GlowBloomCutoff;
		r_envMapSunIntensity->current.value = SunIntensity;
		r_lightTweakSunColor->current.vector = SunColor;
		r_lightTweakSunDirection->current.vector = vec4(SunDirection, 1);
	}
}
