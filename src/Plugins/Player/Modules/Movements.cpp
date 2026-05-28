#include "Movements.hpp"

#include "Player/Movements/CS.hpp"
#include "Player/Movements/Q3.hpp"

constexpr const char* modes = "CoD4\0Q3\0Q3 CPM\0CS\0";

namespace IW3SR::Addons
{
	Movements::Movements() : Module("sr.player.movements", "Player", "Movements")
	{
		BhopText = Text("BHOP", FONT_SPACERANGER, -35, 8, 0.8, { 1, 1, 1, 1 });
		BhopText.SetRectAlignment(Horizontal::Center, Vertical::Center);
		BhopText.SetAlignment(Alignment::Center, Alignment::Bottom);

		KeyBhop = Bind(Key_Space);
		KeyBhopToggle = Bind(Input_None);

		UseBhop = false;
		UseBhopToggle = false;
		UseInterpolateMovers = true;
		BhopToggled = false;
	}

	void Movements::Menu()
	{
		if (Dvar::Get<bool>("sv_running"))
		{
			static MovementMode mode = MovementMode::COD4;
			if (ImGui::Combo("Movements", reinterpret_cast<int*>(&mode), modes))
			{
				switch (mode)
				{
				case MovementMode::COD4:
					SetHardLanding(true);
					statData->stats.data.bytedata[1700] = 1;
					Dvar::Set<int>("g_speed", 190);
					Dvar::Set<float>("g_gravity", 800.0f);
					Dvar::Set<float>("jump_height", 39.0f);
					Dvar::Set<float>("bg_falldamageminheight", 128.0f);
					Dvar::Set<float>("bg_falldamagemaxheight", 300.0f);
					Dvar::Set<float>("bg_bobMax", 8.0f);
					Dvar::Set<float>("friction", 5.5f);
					break;
				case MovementMode::Q3:
					statData->stats.data.bytedata[1700] = 3;
					SetHardLanding(false);
					Dvar::Set<int>("g_speed", 320);
					Dvar::Set<float>("g_gravity", 800.0f);
					Dvar::Set<float>("jump_height", 39.0f);
					Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
					Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
					Dvar::Set<float>("bg_bobMax", 0.0f);
					Dvar::Set<float>("friction", 6.0f);
					break;
				case MovementMode::Q3_CPM:
					statData->stats.data.bytedata[1700] = 4;
					SetHardLanding(false);
					Dvar::Set<int>("g_speed", 320);
					Dvar::Set<float>("g_gravity", 800.0f);
					Dvar::Set<float>("jump_height", 39.0f);
					Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
					Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
					Dvar::Set<float>("bg_bobMax", 0.0f);
					Dvar::Set<float>("friction", 6.0f);
					break;
				case MovementMode::CS:
					statData->stats.data.bytedata[1700] = 5;
					SetHardLanding(false);
					Dvar::Set<int>("g_speed", 250);
					Dvar::Set<float>("g_gravity", 800.0f);
					Dvar::Set<float>("jump_height", 39.0f);
					Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
					Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
					Dvar::Set<float>("bg_bobMax", 8.0f);
					Dvar::Set<float>("friction", 5.5f);
					break;
				}
			}
		}
		ImGui::Checkbox("Interpolate Movers", &UseInterpolateMovers);
		ImGui::Tooltip("Smooth camera interpolation on moving and rotating platforms.");

		ImGui::Checkbox("Bhop", &UseBhop);
		ImGui::SameLine();
		ImGui::Keybind("##BhopKey", &KeyBhop.Input);

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
		switch (GetMovementMode())
		{
		case MovementMode::Q3:
			event.PreventDefault = true;
			Q3::WalkMove(event.pm, event.pml);
			break;
		case MovementMode::Q3_CPM:
			event.PreventDefault = true;
			Q3::WalkMoveCPM(event.pm, event.pml);
			break;
		case MovementMode::CS:
			event.PreventDefault = true;
			CS::WalkMove(event.pm, event.pml);
			break;
		}
	}

	void Movements::OnAirMove(EventPMoveAir& event)
	{
		switch (GetMovementMode())
		{
		case MovementMode::Q3:
			event.PreventDefault = true;
			Q3::AirMove(event.pm, event.pml);
			break;
		case MovementMode::Q3_CPM:
			event.PreventDefault = true;
			Q3::AirMoveCPM(event.pm, event.pml);
			break;
		case MovementMode::CS:
			event.PreventDefault = true;
			CS::AirMove(event.pm, event.pml);
			break;
		}
	}

	void Movements::OnGroundTrace(EventPMoveGroundTrace& event)
	{
		switch (GetMovementMode())
		{
		case MovementMode::Q3:
		case MovementMode::Q3_CPM:
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

		if (UseBhop && KeyBhop.IsDown())
		{
			bool inMantle = ps->pm_flags & PMF_MANTLE;
			bool inLadder = ps->pm_flags & PMF_LADDER;
			bool mantleAvailable = ps->mantleState.flags & 8;

			if (PMove::OnGround())
			{
				// Set jump only when 500ms cooldown has expired, otherwise Jump_Check returns
				if (ps->commandTime - ps->jumpTime >= 500)
				{
					clients->stance = CL_STANCE_STAND;
					cmd->buttons &= ~(BUTTON_CROUCH | BUTTON_CROUCH_HOLD | BUTTON_PRONE | BUTTON_PRONE_HOLD);
					cmd->buttons |= BUTTON_JUMP;
				}
				// Clear jump during cooldown to keep oldcmd clean for edge detection when it expires
				else
				{
					cmd->buttons &= ~BUTTON_JUMP;
				}
			}
			// Clear jump while in air so oldcmd is clean on landing, preserve on mantle and ladder
			else if (!inMantle && !inLadder && !mantleAvailable)
			{
				cmd->buttons &= ~(BUTTON_JUMP | BUTTON_SPRINT);
			}
		}
		if (BhopToggled && PMove::OnGround())
		{
			cmd->buttons |= BUTTON_JUMP;
			BhopToggled = false;
		}
	}

	void Movements::SetHardLanding(bool state)
	{
		if (state)
		{
			// Original code
			Memory::Write(0x410315, "\xD9\x46\x28\xDD\x05");
		}
		else
		{
			// Skip the hard landing section in PM_CrashLand
			Memory::JMP(0x410315, 0x410333);
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

	MovementMode Movements::GetMovementMode()
	{
		switch (statData->stats.data.bytedata[1700]) // sr_mode
		{
		case 1: // 190
		case 2: // 210
			return MovementMode::COD4;
		case 3: // Q3
			return MovementMode::Q3;
		case 4: // Q3 CPM
			return MovementMode::Q3_CPM;
		case 5: // CS
		case 6: // Portal
			return MovementMode::CS;
		}
		return MovementMode::COD4;
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
