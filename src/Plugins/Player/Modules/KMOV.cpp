#include "KMOV.hpp"

#include <random>

constexpr const char* types = "Player\0FPS\0Velocity\0Map\0Timer\0Health\0Hook\0";

namespace IW3SR::Addons
{
	KMOV::KMOV() : Module("sr.player.kmov", "Player", "KMOV")
	{
		NodeLT.Element = Text("0", FONT_SPACERANGER, 20, -100, 2, { 1, 1, 1, 1 });
		NodeLT.Element.SetRectAlignment(Horizontal::Left, Vertical::Bottom);
		NodeLT.Element.SetAlignment(Alignment::Left, Alignment::Bottom);
		NodeLT.Element.Skew.y = -0.1f;
		NodeLT.Type = NodeEnum::Player;

		NodeLB.Element = Text("0", FONT_SPACERANGER, 20, -75, 2, { 1, 1, 1, 1 });
		NodeLB.Element.SetRectAlignment(Horizontal::Left, Vertical::Bottom);
		NodeLB.Element.SetAlignment(Alignment::Left, Alignment::Bottom);
		NodeLB.Element.Skew.y = -0.1f;
		NodeLB.Type = NodeEnum::Timer;

		NodeRT.Element = Text("0", FONT_SPACERANGER, -20, -185, 2, { 0, 1, 1, 1 });
		NodeRT.Element.SetRectAlignment(Horizontal::Right, Vertical::Bottom);
		NodeRT.Element.SetAlignment(Alignment::Right, Alignment::Bottom);
		NodeRT.Element.Skew.y = 0.1f;
		NodeRT.Type = NodeEnum::FPS;

		NodeRB.Element = Text("0", FONT_SPACERANGER, -20, -160, 2, { 1, 1, 1, 1 });
		NodeRB.Element.SetRectAlignment(Horizontal::Right, Vertical::Bottom);
		NodeRB.Element.SetAlignment(Alignment::Right, Alignment::Bottom);
		NodeRB.Element.Skew.y = 0.1f;
		NodeRB.Type = NodeEnum::Velocity;
	}

	void KMOV::Initialize()
	{
		if (!glm::length2(CurrentOffset))
		{
			NodeLT.OriginalPosition = NodeLT.Element.Position;
			NodeLB.OriginalPosition = NodeLB.Element.Position;
			NodeRT.OriginalPosition = NodeRT.Element.Position;
			NodeRB.OriginalPosition = NodeRB.Element.Position;
		}
		NodeLT.Element.Position = NodeLT.OriginalPosition;
		NodeLB.Element.Position = NodeLB.OriginalPosition;
		NodeRT.Element.Position = NodeRT.OriginalPosition;
		NodeRB.Element.Position = NodeRB.OriginalPosition;

		CurrentOffset = { 0, 0 };
	}

	void KMOV::Menu()
	{
		ImGui::DragFloat("Jump Power", &JumpPower, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Angles Power", &AnglesPower, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Fire Power", &FirePower, 0.1f, 0.0f, 10.0f);

		if (ImGui::CollapsingHeader("LT"))
			MenuNode(NodeLT);
		if (ImGui::CollapsingHeader("LB"))
			MenuNode(NodeLB);
		if (ImGui::CollapsingHeader("RT"))
			MenuNode(NodeRT);
		if (ImGui::CollapsingHeader("RB"))
			MenuNode(NodeRB);
	}

	void KMOV::MenuNode(Node& node)
	{
		ImGui::Combo("Type", reinterpret_cast<int*>(&node.Type), types);
		if (node.Type == NodeEnum::Hook)
		{
			ImGui::InputInt("Hook HUD", &node.Hook);
			ImGui::InputText("Hook Value", &node.HookString);
			ImGui::Tooltip("Display the value of a HUD element. Insert '{}' where you want the value to appear.");
			ImGui::Checkbox("Hook Float", &node.HookFloat);
		}
		node.Element.Menu("Text");
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
		const bool isFire = pmove->cmd.buttons & KEY_MASK_FIRE;
		const bool isAds = pmove->cmd.buttons & KEY_MASK_ADS;

		// Q3
		if (isFire && (ps.weapon == q3rl || ps.weapon == q3pg))
		{
			if (ps.weapon == q3rl)
				FireDuration = 0.5f;
			if (ps.weapon == q3pg)
				FireDuration = 0.1f;

			IsShaking = true;
		}
		// Portal
		else if ((isFire || isAds) && ps.weapon == portal)
		{
			FireDuration = 0.2f;
			IsShaking = true;
		}
		// RPG
		else if (ps.weapon == rpg && ps.weaponstate == 5 && !ps.weaponTime)
		{
			FireDuration = 0.5f;
			IsShaking = true;
		}
		// Weapons
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
		float x = (AnglesDelta[1] * AnglesPower) * UI::DeltaTime();
		float y = (AnglesDelta[0] * AnglesPower) * UI::DeltaTime();
		x = std::clamp(x, -0.5f, 0.5f);
		y = std::clamp(y, -0.5f, 0.5f);
		return { x, y };
	}

	float KMOV::Jump()
	{
		const float jumpMax = 40;
		float jumpOrigin = std::min(std::max(JumpOrigin, -jumpMax), jumpMax);
		float multiplier = IsBouncing ? 0.5f : 1.0f;
		float jumpValue = ((JumpOrigin * multiplier) * JumpPower) * UI::DeltaTime();
		return std::clamp(-jumpValue, -0.5f, 0.5f);
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

		x *= FirePower * damper * fadeIn;
		y *= FirePower * damper * fadeIn;

		x = std::clamp(x, -0.1f, 0.1f);
		y = std::clamp(y, -0.1f, 0.1f);

		if (elapsed >= FireDuration)
		{
			elapsed = 0;
			IsShaking = false;
		}
		return { x, y };
	}

	void KMOV::RenderNode(Node& node)
	{
		node.Element.Value = "";

		switch (node.Type)
		{
		case NodeEnum::Player:
			node.Element.Value = std::string(Dvar::Get<const char*>("name"));
			break;
		case NodeEnum::FPS:
			node.Element.Value = std::format("{} FPS", Dvar::Get<int>("com_maxfps"));
			break;
		case NodeEnum::Velocity:
			node.Element.Value = std::format("{} UPS", static_cast<int>(glm::length(vec2(pmove->ps->velocity))));
			break;
		case NodeEnum::Map:
			node.Element.Value = std::string(Dvar::Get<const char*>("mapname"));
			break;
		case NodeEnum::Timer:
		{
			float seconds = UI::Time() - StartTime;
			int minutes = static_cast<int>(seconds) / 60;
			float secs = fmod(seconds, 60.0f);
			char buffer[16];
			snprintf(buffer, sizeof(buffer), "%d:%06.3f", minutes, secs);
			node.Element.Value = std::string(buffer);
			break;
		}
		case NodeEnum::Health:
			node.Element.Value = std::format("{} HP", Player::Self()->info->health);
			break;
		case NodeEnum::Hook:
		{
			const auto& ps = cgs->predictedPlayerState;
			const hudelem_s* huds = node.Hook < 31 ? ps.hud.current : ps.hud.archival;
			int index = node.Hook < 31 ? node.Hook : node.Hook - 31;
			std::string value = "";
			switch (huds[index].type)
			{
			case HE_TYPE_TEXT:
				value = ""; // GetHudElemInfo
				break;
			case HE_TYPE_VALUE:
				value = node.HookFloat ? std::to_string(huds[index].value)
									   : std::to_string(static_cast<int>(huds[index].value));
				break;
			}
			try
			{
				node.Element.Value = std::vformat(node.HookString, std::make_format_args(value));
			}
			catch (const std::format_error& e)
			{
				node.Element.Value = node.HookString;
			}
			break;
		}
		}
		node.Element.Position = node.OriginalPosition + CurrentOffset;
		node.Element.Render();
	}

	void KMOV::OnSpawn(EventClientSpawn& event)
	{
		StartTime = UI::Time();
		Initialize();
	}

	void KMOV::OnRender()
	{
		Compute();

		const auto fire = Fire();
		const auto jump = Jump();
		const auto angles = Angles();

		CurrentOffset.x += fire.x;
		CurrentOffset.y += fire.y;
		CurrentOffset.y += jump;
		CurrentOffset.x += angles.x;

		const float maxOffset = 10.0f;
		CurrentOffset.x = std::clamp(CurrentOffset.x, -maxOffset, maxOffset);
		CurrentOffset.y = std::clamp(CurrentOffset.y, -maxOffset, maxOffset);

		const float lerpSpeed = 3.0f;
		float deltaTime = std::clamp(UI::DeltaTime(), 0.0f, 0.1f);
		float t = 1.0f - std::exp(-lerpSpeed * deltaTime);
		CurrentOffset.x *= (1.0f - t);
		CurrentOffset.y *= (1.0f - t);

		RenderNode(NodeLT);
		RenderNode(NodeLB);
		RenderNode(NodeRT);
		RenderNode(NodeRB);
	}
}
