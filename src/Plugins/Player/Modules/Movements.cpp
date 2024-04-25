#include "Movements.hpp"
#include "Movements/CS.hpp"
#include "Movements/Q3.hpp"

namespace IW3SR::Addons
{
	Movements::Movements() : Module("sr.player.movements", "Player", "Movements")
	{
		BhopText = Text("BHOP", FONT_SPACERANGER, -35, 8, 0.8, { 1, 1, 1, 1 });
		BhopText.SetRectAlignment(HORIZONTAL_CENTER, VERTICAL_CENTER);
		BhopText.SetAlignment(ALIGN_CENTER, ALIGN_BOTTOM);

		Mode = MovementMode::COD4;
		BhopKey = Keyboard(Key_G);

		UseBhop = false;
		UseBhopToggle = false;
		UseInterpolateMovers = true;
	}

	void Movements::OnMenu()
	{
		if (ImGui::RadioButton("CoD4", reinterpret_cast<int*>(&Mode), 0))
		{
			Dvar::Set<int>("g_speed", 190);
			Dvar::Set<float>("g_gravity", 800.0f);
			Dvar::Set<float>("jump_height", 39.0f);
			Dvar::Set<float>("bg_falldamageminheight", 128.0f);
			Dvar::Set<float>("bg_falldamagemaxheight", 300.0f);
			Dvar::Set<float>("bg_bobMax", 8.0f);
			Dvar::Set<float>("friction", 5.5f);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Q3", reinterpret_cast<int*>(&Mode), 1))
		{
			Dvar::Set<int>("g_speed", 320);
			Dvar::Set<float>("g_gravity", 800.0f);
			Dvar::Set<float>("jump_height", 46.0f);
			Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
			Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
			Dvar::Set<float>("bg_bobMax", 0.0f);
			Dvar::Set<float>("friction", 8.0f);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("CS", reinterpret_cast<int*>(&Mode), 2))
		{
			Dvar::Set<int>("g_speed", 190);
			Dvar::Set<float>("g_gravity", 800.0f);
			Dvar::Set<float>("jump_height", 39.0f);
			Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
			Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
			Dvar::Set<float>("bg_bobMax", 8.0f);
			Dvar::Set<float>("friction", 5.5f);
		}
		ImGui::SameLine();
		ImGui::TextWrapped("Movements modifications also works if you are hosting a server with IW3SR.");
		ImGui::Checkbox("Interpolate Movers", &UseInterpolateMovers);

		ImGui::Checkbox("Bhop", &UseBhop);
		ImGui::Keybind("Bhop Landing", &BhopKey.Key);
		BhopText.Menu("Bhop Indicator");
	}

	void Movements::OnPredict(EventClientPredict& event)
	{
		InterpolateViewForMover();
	}

	void Movements::OnWalkMove(EventPMoveWalk& event)
	{
		switch (Mode)
		{
		case MovementMode::Q3:
			event.PreventDefault = true;
			Q3::WalkMove(event.pm, event.pml);
			break;
		}
	}

	void Movements::OnAirMove(EventPMoveAir& event)
	{
		switch (Mode)
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

	void Movements::OnFinishMove(EventPMoveFinish& event)
	{
		Bhop(event.cmd);
	}

	void Movements::Bhop(usercmd_s* cmd)
	{
		if (BhopKey.IsPressed())
			UseBhopToggle = true;

		if (UseBhop && (cmd->buttons & BUTTON_JUMP))
		{
			usercmd_s* oldcmd = &clients->cmds[clients->cmdNumber & 0x7F];
			if (cmd->buttons & BUTTON_JUMP && oldcmd->buttons & BUTTON_JUMP)
				cmd->buttons -= BUTTON_JUMP;
		}
		if (UseBhopToggle && PMove::OnGround())
		{
			cmd->buttons |= BUTTON_JUMP;
			UseBhopToggle = false;
		}
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

	void Movements::OnRender()
	{
		if (UseBhopToggle)
			BhopText.Render();
	}
}
