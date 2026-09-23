#include "Tweaks.hpp"

namespace IW3SR::Addons
{
	Tweaks::Tweaks() : Module("sr.graphics.tweaks", "Graphics", "Tweaks")
	{
		DrawTweaks = false;
		DrawGlow = false;
		DrawSun = true;
		SunOverride = false;

		TweakBrightness = 0;
		TweakContrast = 1;
		TweakDesaturation = 0;
		TweakLightTint = { 1, 1, 1 };
		TweakDarkTint = { 1, 1, 1 };

		GlowRadius = 5;
		GlowBloomDesaturation = 0;
		GlowBloomIntensity = 1;
		GlowBloomCutoff = 0.5;

		SunIntensity = 2;
		SunColor = { 1, 1, 1, 1 };
		SunDirection = { 270, 90, 0 };
	}

	// Hands film and glow back to the vision set, which is where they come from when nothing overrides them.
	void Tweaks::Release()
	{
		if (const auto dvar = Dvar::Find("r_filmUseTweaks"))
			dvar->current.enabled = false;
		if (const auto dvar = Dvar::Find("r_glowUseTweaks"))
			dvar->current.enabled = false;
		if (const auto dvar = Dvar::Find("r_drawSun"))
			dvar->current.enabled = true;
	}

	void Tweaks::Menu()
	{
		if (ImGui::CollapsingHeader("Tweaks"))
		{
			ImGui::PushID("Tweaks");
			ImGui::Checkbox("Enabled", &DrawTweaks);
			ImGui::SliderFloat("Brightness", &TweakBrightness, -1, 1);
			ImGui::SliderFloat("Contrast", &TweakContrast, 0, 4);
			ImGui::SliderFloat("Desaturation", &TweakDesaturation, 0, 1);
			ImGui::ColorEdit3("Light Tint", &TweakLightTint.x, ImGuiColorEditFlags_Float);
			ImGui::ColorEdit3("Dark Tint", &TweakDarkTint.x, ImGuiColorEditFlags_Float);
			ImGui::PopID();
		}
		if (ImGui::CollapsingHeader("Glow"))
		{
			ImGui::PushID("Glow");
			ImGui::Checkbox("Enabled", &DrawGlow);
			ImGui::SliderFloat("Radius", &GlowRadius, 0, 32);
			ImGui::SliderFloat("Bloom Desaturation", &GlowBloomDesaturation, 0, 1);
			ImGui::SliderFloat("Bloom Intensity", &GlowBloomIntensity, 0, 20);
			ImGui::SliderFloat("Bloom Cut-off", &GlowBloomCutoff, 0, 1);
			ImGui::PopID();
		}
		if (ImGui::CollapsingHeader("Sun"))
		{
			ImGui::PushID("Sun");
			ImGui::Checkbox("Enabled", &DrawSun);
			ImGui::Checkbox("Override", &SunOverride);
			ImGui::Tooltip("Replace the map's own sun with the values below.\n"
						   "Turning it off gives the map's sun back on the next map load.");
			ImGui::SliderFloat("Intensity", &SunIntensity, 0, 4);
			ImGui::ColorEdit4("Color", &SunColor.x, ImGuiColorEditFlags_Float);
			ImGui::SliderFloat3("Direction", &SunDirection.x, -360, 360);
			ImGui::PopID();
		}
	}

	// Only a group that is switched on writes its dvars, so enabling the module for one of them leaves
	// the map's own film, bloom and sun alone.
	void Tweaks::OnRender()
	{
		static const auto r_filmTweakEnable = Dvar::Find("r_filmTweakEnable");
		static const auto r_filmUseTweaks = Dvar::Find("r_filmUseTweaks");
		static const auto r_glowUseTweaks = Dvar::Find("r_glowUseTweaks");
		static const auto r_glowTweakEnable = Dvar::Find("r_glowTweakEnable");
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

		r_filmUseTweaks->current.enabled = DrawTweaks;
		r_glowUseTweaks->current.enabled = DrawGlow;
		r_drawSun->current.enabled = DrawSun;

		if (DrawTweaks)
		{
			r_filmTweakEnable->current.enabled = true;
			r_filmTweakBrightness->current.value = TweakBrightness;
			r_filmTweakContrast->current.value = TweakContrast;
			r_filmTweakDesaturation->current.value = TweakDesaturation;
			r_filmTweakLightTint->current.vector = vec4(TweakLightTint, 1);
			r_filmTweakDarkTint->current.vector = vec4(TweakDarkTint, 1);
		}
		if (DrawGlow)
		{
			r_glowTweakEnable->current.enabled = true;
			r_glowTweakRadius0->current.value = GlowRadius;
			r_glowTweakBloomDesaturation->current.value = GlowBloomDesaturation;
			r_glowTweakBloomIntensity0->current.value = GlowBloomIntensity;
			r_glowTweakBloomCutoff->current.value = GlowBloomCutoff;
		}
		if (SunOverride)
			ApplySun();
	}

	// The renderer only rebuilds the sun when these dvars are flagged modified, so they are written only
	// when they differ: every frame would rebuild it every frame. A map load resets them from the map,
	// which is what makes them differ again and the override come back.
	void Tweaks::ApplySun()
	{
		static const auto r_envMapSunIntensity = Dvar::Find("r_envMapSunIntensity");
		static const auto r_lightTweakSunColor = Dvar::Find("r_lightTweakSunColor");
		static const auto r_lightTweakSunDirection = Dvar::Find("r_lightTweakSunDirection");

		if (r_envMapSunIntensity->current.value != SunIntensity)
		{
			r_envMapSunIntensity->current.value = SunIntensity;
			r_envMapSunIntensity->modified = true;
		}

		// A colour dvar holds four packed bytes, not floats.
		char color[4];
		for (int i = 0; i < 4; i++)
			color[i] = static_cast<char>(std::clamp(SunColor[i], 0.0f, 1.0f) * 255.0f + 0.5f);

		if (std::memcmp(r_lightTweakSunColor->current.color, color, sizeof(color)))
		{
			std::memcpy(r_lightTweakSunColor->current.color, color, sizeof(color));
			r_lightTweakSunColor->modified = true;
		}

		if (vec3(r_lightTweakSunDirection->current.vector) != SunDirection)
		{
			r_lightTweakSunDirection->current.vector = vec4(SunDirection, 1);
			r_lightTweakSunDirection->modified = true;
		}
	}
}
