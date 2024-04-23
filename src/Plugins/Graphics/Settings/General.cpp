#include "General.hpp"

namespace IW3SR::Addons
{
	General::General() : Setting("sr.graphics.general", "Graphics", "General")
	{
		DrawSpecular = true;
		DrawFog = false;
		DrawDecals = true;
		WaterAnimation = false;
		DrawTweaks = false;
		DrawGlow = false;
		DrawSun = false;

		SunIntensity = 1;
		SpecularColorScale = 1;
		TweakBrightness = 0;
		TweakDesaturation = 0.2;
		GlowRadius = 5;
		GlowBloomDesaturation = 0;
		GlowBloomIntensity = 1;
		GlowBloomCutoff = 0.5;

		SunColor = { 1, 0.749, 0, 1 };
		SunDirection = { 0, 0, 0 };
		TweakLightTint = { 1.1, 1.05, 0.85 };
		TweakDarkTint = { 0.7, 0.85, 1 };
	}

	void General::OnMenu()
	{
		if (ImGui::CollapsingHeader("General"))
		{
			ImGui::Text("Specular");
			ImGui::Separator();
			ImGui::Toggle("Enabled", &DrawSpecular);
			ImGui::SliderFloat("Specular color scale", &SpecularColorScale, 0, 100);

			ImGui::Text("Misc");
			ImGui::Separator();
			ImGui::Toggle("fog", &DrawFog);
			ImGui::SameLine();
			ImGui::Text("Fog");
			ImGui::Toggle("decals", &DrawDecals);
			ImGui::SameLine();
			ImGui::Text("Decals");
			ImGui::Toggle("water anim", &WaterAnimation);
			ImGui::SameLine();
			ImGui::Text("Water animation");
		}
		if (ImGui::CollapsingHeader("Tweaks"))
		{
			ImGui::Toggle("film tweaks", &DrawTweaks);
			ImGui::SameLine();
			ImGui::Text("Enabled");
			ImGui::SliderFloat("Brightness", &TweakBrightness, -1, 1);
			ImGui::SliderFloat("Desaturation", &TweakDesaturation, 0, 1);
			ImGui::ColorEdit3("Light tint", TweakLightTint, ImGuiColorEditFlags_Float);
			ImGui::ColorEdit3("Dark tint", TweakDarkTint, ImGuiColorEditFlags_Float);
		}
		if (ImGui::CollapsingHeader("Glow"))
		{
			ImGui::Toggle("glow", &DrawGlow);
			ImGui::SameLine();
			ImGui::Text("Enabled");
			ImGui::SliderFloat("Radius", &GlowRadius, 0, 32);
			ImGui::SliderFloat("Bloom desaturation", &GlowBloomDesaturation, 0, 1);
			ImGui::SliderFloat("Bloom intensity", &GlowBloomIntensity, 0, 20);
			ImGui::SliderFloat("Bloom cut-off", &GlowBloomCutoff, 0, 1);
		}
		if (ImGui::CollapsingHeader("Sun"))
		{
			ImGui::Toggle("sun", &DrawSun);
			ImGui::SameLine();
			ImGui::Text("Enabled");
			ImGui::SliderFloat("Sun intensity", &SunIntensity, 0, 4);
			ImGui::ColorEdit4("Sun color", SunColor, ImGuiColorEditFlags_Float);
			ImGui::SliderFloat3("Sun direction", SunDirection, -360, 360);
		}
		if (ImGui::Button("Apply changes"))
		{
			Dvar::Set<bool>("r_specular", DrawSpecular);
			Dvar::Set<bool>("r_fog", DrawFog);
			Dvar::Set<bool>("r_drawDecals", DrawDecals);
			Dvar::Set<bool>("r_drawWater", WaterAnimation);
			Dvar::Set<bool>("r_filmTweakEnable", DrawTweaks);
			Dvar::Set<bool>("r_glow", DrawGlow);
			Dvar::Set<bool>("r_drawSun", DrawSun);

			Dvar::Set<float>("r_envMapSunIntensity", SunIntensity);
			Dvar::Set<float>("r_specularColorScale", SpecularColorScale);
			Dvar::Set<float>("r_filmTweakBrightness", TweakBrightness);
			Dvar::Set<float>("r_filmTweakDesaturation", TweakDesaturation);
			Dvar::Set<float>("r_glowTweakRadius0", GlowRadius);
			Dvar::Set<float>("r_glowTweakBloomDesaturation", GlowBloomDesaturation);
			// Dvar::Set<float>("r_glowTweakBloomIntensity", GlowBloomIntensity);
			Dvar::Set<float>("r_glowTweakBloomCutoff", GlowBloomCutoff);

			Dvar::Set<vec4>("r_lightTweakSunColor", SunColor);
			Dvar::Set<vec3>("r_lightTweakSunDirection", SunDirection);
			Dvar::Set<vec3>("r_filmTweakLightTint", TweakLightTint);
			Dvar::Set<vec3>("r_filmTweakDarkTint", TweakDarkTint);
		}
	}
}
