#include "Movements.hpp"

constexpr const char* modes = "CoD4\0Q3\0Q3CPM\0CS\0";

namespace IW3SR::Addons
{
	Movements::Movements() : Module("sr.player.movements", "Player", "Movements")
	{
		BhopText = Text("BHOP", FONT_SPACERANGER, -35, 8, 0.8, { 1, 1, 1, 1 });
		BhopText.SetRectAlignment(Horizontal::Center, Vertical::Center);
		BhopText.SetAlignment(Alignment::Center, Alignment::Bottom);

		KeyBhop = Bind(Key_Space);
		KeyBhopToggle = Bind(Input_None);
		KeyTurnLeft = Bind(Input_None);
		KeyTurnRight = Bind(Input_None);

		UseBhop = false;
		UseBhopToggle = false;
		UseTurnBind = false;
		BhopToggled = false;
	}

	void Movements::Menu()
	{
		if (Dvar::Get<bool>("sv_running"))
		{
			static MovementMode mode = PMove::GetMovementMode();
			if (ImGui::Combo("Movements", reinterpret_cast<int*>(&mode), modes))
			{
				switch (mode)
				{
				case MovementMode::COD4:
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
					Dvar::Set<int>("g_speed", 320);
					Dvar::Set<float>("g_gravity", 800.0f);
					Dvar::Set<float>("jump_height", 39.0f);
					Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
					Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
					Dvar::Set<float>("bg_bobMax", 0.0f);
					Dvar::Set<float>("friction", 6.0f);
					break;
				case MovementMode::Q3CPM:
					statData->stats.data.bytedata[1700] = 4;
					Dvar::Set<int>("g_speed", 320);
					Dvar::Set<float>("g_gravity", 800.0f);
					Dvar::Set<float>("jump_height", 39.0f);
					Dvar::Set<float>("bg_falldamageminheight", 99998.0f);
					Dvar::Set<float>("bg_falldamagemaxheight", 99999.0f);
					Dvar::Set<float>("bg_bobMax", 0.0f);
					Dvar::Set<float>("friction", 6.0f);
					break;
				case MovementMode::CS:
					statData->stats.data.bytedata[1700] = 6;
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
		ImGui::Checkbox("Bhop", &UseBhop);
		ImGui::SameLine();
		ImGui::Keybind("##KeyBhop", &KeyBhop.Input);

		ImGui::Checkbox("Bhop Toggle", &UseBhopToggle);
		ImGui::SameLine();
		ImGui::Keybind("##KeyBhopToggle", &KeyBhopToggle.Input);

		ImGui::Checkbox("CS Turnbind", &UseTurnBind);
		ImGui::SameLine();
		ImGui::Keybind("##KeyTurnLeft", &KeyTurnLeft.Input);
		ImGui::SameLine();
		ImGui::Keybind("##KeyTurnRight", &KeyTurnRight.Input);
	}

	void Movements::OnFinishMove(EventPMoveFinish& event)
	{
		Bhop(event.ps, event.cmd);
		TurnBind(event.ps, event.cmd);
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

	void Movements::TurnBind(playerState_s* ps, usercmd_s* cmd)
	{
		if (!UseTurnBind)
			return;

		if (!KeyTurnLeft.IsDown() && !KeyTurnRight.IsDown())
			return;

		const float cl_yawspeed = Dvar::Get<float>("cl_yawspeed");
		const float speed = cls->frametime;
		const float deltaTime = speed * 0.001f;

		cmd->rightmove = KeyTurnRight.IsDown() ? 127 : KeyTurnLeft.IsDown() ? -127 : 0;
		clients->viewangles[1] += KeyTurnLeft.IsDown() * cl_yawspeed * deltaTime;
		clients->viewangles[1] -= KeyTurnRight.IsDown() * cl_yawspeed * deltaTime;
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
