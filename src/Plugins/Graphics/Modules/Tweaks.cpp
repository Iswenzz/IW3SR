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
			ImGui::ColorEdit3("Light Tint", TweakLightTint, ImGuiColorEditFlags_Float);
			ImGui::ColorEdit3("Dark Tint", TweakDarkTint, ImGuiColorEditFlags_Float);
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
			ImGui::ColorEdit4("Color", SunColor, ImGuiColorEditFlags_Float);
			ImGui::SliderFloat3("Direction", SunDirection, -360, 360);
		}
	}

	void Tweaks::OnRender()
	{
		Dvar::Set<bool>("r_filmTweakEnable", DrawTweaks);
		Dvar::Set<bool>("r_filmUseTweaks", DrawTweaks);
		Dvar::Set<bool>("r_glow", DrawGlow);
		Dvar::Set<bool>("r_glowUseTweaks", DrawGlow);
		Dvar::Set<bool>("r_drawSun", DrawSun);

		Dvar::Set<float>("r_filmTweakBrightness", TweakBrightness);
		Dvar::Set<float>("r_filmTweakContrast", TweakContrast);
		Dvar::Set<float>("r_filmTweakDesaturation", TweakDesaturation);
		Dvar::Set<vec3>("r_filmTweakLightTint", TweakLightTint);
		Dvar::Set<vec3>("r_filmTweakDarkTint", TweakDarkTint);

		Dvar::Set<float>("r_glowTweakRadius0", GlowRadius);
		Dvar::Set<float>("r_glowTweakBloomDesaturation", GlowBloomDesaturation);
		Dvar::Set<float>("r_glowTweakBloomIntensity", GlowBloomIntensity);
		Dvar::Set<float>("r_glowTweakBloomCutoff", GlowBloomCutoff);

		Dvar::Set<float>("r_envMapSunIntensity", SunIntensity);
		Dvar::Set<vec4>("r_lightTweakSunColor", SunColor);
		Dvar::Set<vec3>("r_lightTweakSunDirection", SunDirection);
	}
}
