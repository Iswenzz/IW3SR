#include "KMOV.hpp"

#include <random>

constexpr const char* displays = "Player\0Mode\0FPS\0Velocity\0Map\0Time\0Health\0";

namespace IW3SR::Addons
{
	KMOV::KMOV() : Module("sr.player.kmov", "Player", "KMOV")
	{
		TextLT = Text("0", FONT_SPACERANGER, 20, -100, 2, { 1, 1, 1, 1 });
		TextLB = Text("0", FONT_SPACERANGER, 20, -75, 2, { 1, 1, 1, 1 });
		TextRT = Text("0", FONT_SPACERANGER, -20, -185, 2, { 0, 1, 1, 1 });
		TextRB = Text("0", FONT_SPACERANGER, -20, -160, 2, { 1, 1, 1, 1 });
	}

	void KMOV::Initialize()
	{
		TextLT.Position = vec2(20, -100);
		TextLT.FontSize = 2;
		TextLT.Skew.y = -0.1f;
		TextLT.SetRectAlignment(HORIZONTAL_LEFT, VERTICAL_BOTTOM);
		TextLT.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		OriginalPositionLT = TextLT.Position;

		TextLB.Position = vec2(20, -75);
		TextLB.FontSize = 2;
		TextLB.Skew.y = -0.1f;
		TextLB.SetRectAlignment(HORIZONTAL_LEFT, VERTICAL_BOTTOM);
		TextLB.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		OriginalPositionLB = TextLB.Position;

		TextRT.Position = vec2(-20, -185);
		TextRT.FontSize = 2;
		TextRT.Skew.y = 0.1f;
		TextRT.SetRectAlignment(HORIZONTAL_RIGHT, VERTICAL_BOTTOM);
		TextRT.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
		OriginalPositionRT = TextRT.Position;

		TextRB.Position = vec2(-20, -160);
		TextRB.FontSize = 2;
		TextRB.Skew.y = 0.1f;
		TextRB.SetRectAlignment(HORIZONTAL_RIGHT, VERTICAL_BOTTOM);
		TextRB.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
		OriginalPositionRB = TextRB.Position;

		CurrentOffset = { 0, 0 };
	}

	void KMOV::Menu()
	{
		if (ImGui::CollapsingHeader("Displays"))
		{
			ImGui::Combo("Display LT", reinterpret_cast<int*>(&DisplayLT), displays);
			ImGui::Combo("Display LB", reinterpret_cast<int*>(&DisplayLB), displays);
			ImGui::Combo("Display RT", reinterpret_cast<int*>(&DisplayRT), displays);
			ImGui::Combo("Display RB", reinterpret_cast<int*>(&DisplayRB), displays);
		}
		if (ImGui::CollapsingHeader("Variables"))
		{
			ImGui::DragFloat("Jump Power", &JumpPower, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Jump Max", &JumpMax, 0.1f, -100.0f, 100.0f);
			ImGui::DragFloat("Camera Power", &CameraPower, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Fire Magnitude", &FireMagnitude, 0.1f, 0.0f, 2.0f);
			ImGui::DragFloat("Fire Speed", &FireSpeed, 0.1f, 0.0f, 100.0f);
		}
		if (ImGui::CollapsingHeader("Texts"))
		{
			TextLT.Menu("LT");
			TextLB.Menu("LB");
			TextRT.Menu("RT");
			TextRB.Menu("RB");
		}
	}

	void KMOV::Compute()
	{
		static vec3 prevOrigin;
		static float prevLandingOrigin;
		static vec3 prevViewAngles;

		auto& ps = cgs->predictedPlayerState;

		// Angles
		for (int i = 0; i < 3; i++)
		{
			AnglesDelta[i] = 0;
			if (ps.viewangles[i] != prevViewAngles[i])
				AnglesDelta[i] = Math::AngleNormalize180(prevViewAngles[i] - ps.viewangles[i]);
		}
		// Jump
		if (ps.groundEntityNum != 1023)
		{
			JumpOrigin = 0;
			LandingOrigin = ps.origin[2];
			IsBouncing = false;
		}
		else
		{
			LandingOrigin = prevLandingOrigin;
			JumpOrigin = ps.origin[2] - prevOrigin[2];
			IsBouncing = !ps.jumpOriginZ;
		}
		// Fire
		const auto rpg = BG_FindWeaponIndexForName("rpg_mp");
		const auto q3rl = BG_FindWeaponIndexForName("gl_ak47_mp");
		const auto q3pg = BG_FindWeaponIndexForName("gl_g3_mp");
		const auto portal = BG_FindWeaponIndexForName("portalgun_mp");
		IsAttacking = pmove->cmd.buttons & 0x1;

		if (IsAttacking && (ps.weapon == q3rl || ps.weapon == q3pg || ps.weapon == portal))
		{
			if (ps.weapon == q3rl)
				FireDuration = 0.5f;
			if (ps.weapon == q3pg)
				FireDuration = 0.1f;
			if (ps.weapon == portal)
				FireDuration = 0.2f;

			IsShaking = true;
		}
		else if (ps.weapon == rpg && ps.weaponstate == 5 && !ps.weaponTime)
		{
			FireDuration = 0.5f;
			IsShaking = true;
		}
		else if (ps.weaponstate == 5)
		{
			FireDuration = ps.weaponTime / 1000.0f;
			IsShaking = true;
		}
		// States
		prevOrigin = ps.origin;
		prevLandingOrigin = LandingOrigin;
		prevViewAngles = ps.viewangles;
	}

	vec2 KMOV::Angles()
	{
		float x = (AnglesDelta[1] * CameraPower) * UI::DeltaTime();
		float y = (AnglesDelta[0] * CameraPower) * UI::DeltaTime();
		return { x * 20, y * 20 };
	}

	float KMOV::Jump()
	{
		double jumpOrigin = std::min(std::max(JumpOrigin, -JumpMax), JumpMax);
		int multiplier = IsBouncing ? -15 : -25;
		return ((JumpOrigin * multiplier) * JumpPower) * UI::DeltaTime();
	}

	vec2 KMOV::Fire()
	{
		static float elapsed = 0;
		static std::mt19937 rand(std::random_device{}());
		static float phaseX = 0;
		static float phaseY = 0;
		static float freqX = 0;
		static float freqY = 0;

		if (!IsShaking)
		{
			elapsed = 0;
			return { 0, 0 };
		}
		if (elapsed == 0)
		{
			std::uniform_real_distribution<float> phaseDist(0.0f, 6.28318f);
			std::uniform_real_distribution<float> freqDist(20.0f, 30.0f);

			phaseX = phaseDist(rand);
			phaseY = phaseDist(rand);
			freqX = freqDist(rand);
			freqY = freqDist(rand);
		}
		elapsed += UI::DeltaTime();
		float percentComplete = elapsed / FireDuration;
		float fadeIn = std::min(elapsed * 20.0f, 1.0f);
		float damper = std::pow(1.0f - percentComplete, 2.0f);
		float time = elapsed;

		float x = std::sin(phaseX + time * freqX) * 0.6f;
		x += std::sin(phaseX * 1.5f + time * freqX * 2.3f) * 0.3f;
		x += std::sin(phaseX * 0.7f + time * freqX * 0.5f) * 0.1f;

		float y = std::sin(phaseY + time * freqY) * 0.6f;
		y += std::sin(phaseY * 1.3f + time * freqY * 1.8f) * 0.3f;
		y += std::sin(phaseY * 0.9f + time * freqY * 0.7f) * 0.1f;

		x *= FireMagnitude * damper * fadeIn * 4;
		y *= FireMagnitude * damper * fadeIn * 4;

		x = std::clamp(x, -0.1f, 0.1f);
		y = std::clamp(y, -0.1f, 0.1f);

		if (elapsed >= FireDuration)
		{
			elapsed = 0;
			IsShaking = false;
		}
		return { x, y };
	}

	const std::string KMOV::ComputeValue(Display display)
	{
		if (display == Display::Player)
			return std::string(Dvar::Get<const char*>("name"));
		if (display == Display::Mode)
			return "Defrag";
		if (display == Display::FPS)
			return std::format("{} FPS", Dvar::Get<int>("com_maxfps"));
		if (display == Display::Velocity)
			return std::format("{} UPS", static_cast<int>(VectorLength2(pmove->ps->velocity)));
		if (display == Display::Map)
			return "";
		if (display == Display::Time)
		{
			float seconds = UI::Time() - StartTime;
			int minutes = static_cast<int>(seconds) / 60;
			float secs = fmod(seconds, 60.0f);
			char buffer[16];
			snprintf(buffer, sizeof(buffer), "%d:%06.3f", minutes, secs);
			return std::string(buffer);
		}
		if (display == Display::Health)
			return std::format("{} HP", Player::Self()->info->health);
		return "";
	}

	void KMOV::OnSpawn(EventClientSpawn& event)
	{
		StartTime = UI::Time();
		Initialize();
	}

	void KMOV::OnRender()
	{
		Compute();

		vec2 offset = Fire();

		Log::WriteLine("{} {}", offset.x, offset.y);

		offset.y += Jump();
		offset.x += Angles().x;

		CurrentOffset.x += offset.x;
		CurrentOffset.y += offset.y;

		const float maxOffset = 10.0f;
		CurrentOffset.x = std::clamp(CurrentOffset.x, -maxOffset, maxOffset);
		CurrentOffset.y = std::clamp(CurrentOffset.y, -maxOffset, maxOffset);

		const float lerpSpeed = 2.0f;
		float t = 1.0f - std::exp(-lerpSpeed * UI::DeltaTime());
		CurrentOffset.x = CurrentOffset.x * (1.0f - t);
		CurrentOffset.y = CurrentOffset.y * (1.0f - t);

		TextLT.Position = OriginalPositionLT + CurrentOffset;
		TextLT.Value = ComputeValue(DisplayLT);
		TextLT.Render();

		TextLB.Position = OriginalPositionLB + CurrentOffset;
		TextLB.Value = ComputeValue(DisplayLB);
		TextLB.Render();

		TextRT.Position = OriginalPositionRT + CurrentOffset;
		TextRT.Value = ComputeValue(DisplayRT);
		TextRT.Render();

		TextRB.Position = OriginalPositionRB + CurrentOffset;
		TextRB.Value = ComputeValue(DisplayRB);
		TextRB.Render();
	}
}
