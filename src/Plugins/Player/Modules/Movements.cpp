#include "Movements.hpp"
#include "Movements/CS.hpp"
#include "Movements/Q3.hpp"

namespace IW3SR::Addons
{
	Movements::Movements() : Module("sr.player.movements", "Player", "Movements")
	{
		BhopText = Text("BHOP", FONT_SPACERANGER, -35, 8, 0.8, { 1, 1, 1, 1 });
		BhopText.SetRectAlignment(Horizontal::Center, Vertical::Center);
		BhopText.SetAlignment(Alignment::Center, Alignment::Bottom);

		KeyBhopToggle = Bind(Input_None);

		UseBhop = false;
		UseBhopToggle = false;
		UseInterpolateMovers = true;
		BhopToggled = false;
	}

	void Movements::Menu()
	{
		int mode = Dvar::Get<int>("pm_mode");

		ImGui::Button(ICON_FA_CIRCLE_INFO);
		ImGui::Tooltip("Use devmap to enable movement modifications.");
		ImGui::BeginDisabled(!Dvar::Get<bool>("sv_running"));
		ImGui::SameLine();

		if (ImGui::RadioButton("CoD4", &mode, 0))
		{
			SetCrashLand(true);

			Dvar::Set<MovementMode>("pm_mode", MovementMode::COD4);
			Dvar::Set<int>("g_speed", 190);
			Dvar::Set<float>("g_gravity", 800.0f);
			Dvar::Set<float>("jump_height", 39.0f);
			Dvar::Set<float>("bg_falldamageminheight", 128.0f);
			Dvar::Set<float>("bg_falldamagemaxheight", 300.0f);
			Dvar::Set<float>("bg_bobMax", 8.0f);
			Dvar::Set<float>("friction", 5.5f);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Q3", &mode, 1))
		{
			SetCrashLand(false);

			Dvar::Set<MovementMode>("pm_mode", MovementMode::Q3);
			Dvar::Set<int>("g_speed", 320);
			Dvar::Set<float>("g_gravity", 800.0f);
			Dvar::Set<float>("jump_height", 46.0f);
			Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
			Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
			Dvar::Set<float>("bg_bobMax", 0.0f);
			Dvar::Set<float>("friction", 8.0f);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("CS", &mode, 2))
		{
			SetCrashLand(false);

			Dvar::Set<MovementMode>("pm_mode", MovementMode::CS);
			Dvar::Set<int>("g_speed", 190);
			Dvar::Set<float>("g_gravity", 800.0f);
			Dvar::Set<float>("jump_height", 39.0f);
			Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
			Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
			Dvar::Set<float>("bg_bobMax", 8.0f);
			Dvar::Set<float>("friction", 5.5f);
		}
		ImGui::EndDisabled();
		ImGui::Checkbox("Interpolate Movers", &UseInterpolateMovers);
		ImGui::Tooltip("Smooth camera interpolation on moving and rotating platforms.");

		ImGui::Checkbox("Bhop", &UseBhop);

		ImGui::Checkbox("Bhop Toggle", &UseBhopToggle);
		ImGui::SameLine();
		ImGui::Keybind("##BhopToggleKey", &KeyBhopToggle.Input);

		BhopText.Menu("Bhop Options");
	}

	void Movements::OnPredict(EventClientPredict& event)
	{
		InterpolateViewForMover();
	}

	void Movements::OnWalkMove(EventPMoveWalk& event)
	{
		switch (Dvar::Get<MovementMode>("pm_mode"))
		{
		case MovementMode::Q3:
			event.PreventDefault = true;
			Q3::WalkMove(event.pm, event.pml);
			break;
		}
	}

	void Movements::OnAirMove(EventPMoveAir& event)
	{
		switch (Dvar::Get<MovementMode>("pm_mode"))
		{
		case MovementMode::Q3:
			event.PreventDefault = true;
			Q3::AirMove(event.pm, event.pml);
			break;
		case MovementMode::CS:
			event.PreventDefault = true;
			CS::AirMove(event.pm, event.pml);
			break;
		}
	}

	void Movements::OnGroundTrace(EventPMoveGroundTrace& event)
	{
		switch (Dvar::Get<MovementMode>("pm_mode"))
		{
		case MovementMode::Q3:
			event.PreventDefault = true;
			Q3::GroundTrace(event.pm, event.pml);
			break;
		}
	}

	void Movements::OnFinishMove(EventPMoveFinish& event)
	{
		Bhop(event.ps, event.cmd);
	}

	void Movements::Bhop(playerState_s* ps, usercmd_s* cmd)
	{
		if (UseBhopToggle && KeyBhopToggle.IsPressed())
			BhopToggled = !BhopToggled;

		if (UseBhop && (cmd->buttons & BUTTON_JUMP))
		{
			usercmd_s* oldcmd = &clients->cmds[clients->cmdNumber - 1 & 0x7F];
			if (cmd->buttons & BUTTON_JUMP && oldcmd->buttons & BUTTON_JUMP)
				cmd->buttons -= BUTTON_JUMP;
		}
		if (BhopToggled && PMove::OnGround())
		{
			cmd->buttons |= BUTTON_JUMP;
			BhopToggled = false;
		}
	}

	void Movements::SetCrashLand(bool state)
	{
		Memory::Write(0x410315, "\xD9\x46\x28\xDD\x05");
		if (!state)
			Memory::JMP(0x410315, 0x410333);
	}

	void Movements::InterpolateViewForMover()
	{
		if (!UseInterpolateMovers)
			return;

		const centity_s* cent = &cg_entities[cgs->predictedPlayerState.groundEntityNum];
		const entityType_t eType = cent->nextState.eType;

		auto viewAngles = cgs->predictedPlayerState.viewangles;
		auto deltaAngles = cgs->predictedPlayerState.delta_angles;
		const int fromTime = cgs->snap->serverTime;
		const int toTime = cgs->time;

		if (eType == ET_SCRIPTMOVER || eType == ET_PLANE)
		{
			vec3 angles, oldAngles;
			BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);
			BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);
			vec3 delta = angles - oldAngles;

			viewAngles[0] += delta.x;
			viewAngles[1] += delta.y;
			viewAngles[2] += delta.z;

			deltaAngles[0] += delta.x;
			deltaAngles[1] += delta.y;
			deltaAngles[2] += delta.z;
		}
	}

	void Movements::OnLoadPosition()
	{
		BhopToggled = false;
	}

	void Movements::OnRender()
	{
		if (BhopToggled)
			BhopText.Render();
	}
}
