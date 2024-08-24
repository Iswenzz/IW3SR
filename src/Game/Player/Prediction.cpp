#include "Prediction.hpp"
#include "Engine/Core/Memory/Memory.hpp"
#include "Game/Definitions/Structs.hpp"
#include "Game/System/Dvar.hpp"
#include "PMove.hpp"

namespace IW3SR
{
	pmove_t Prediction::CreatePM(playerState_s* ps, usercmd_s* cmd)
	{
		usercmd_s* oldcmd = PMove::GetUserCommand(clients->cmdNumber - 1);
		pmove_t pm = { 0 };

		pm.ps = ps;
		pm.cmd = *cmd;
		pm.oldcmd = *oldcmd;

		pm.mins[0] = -15;
		pm.mins[1] = -15;
		pm.mins[2] = 0;
		pm.maxs[0] = 15;
		pm.maxs[1] = 15;
		if (ps->viewHeightCurrent == 60)
			pm.maxs[2] = 70;
		if (ps->viewHeightCurrent == 40)
			pm.maxs[2] = 50;
		if (ps->viewHeightCurrent == 11)
			pm.maxs[2] = 30;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.handler = 1;

		return pm;
	}

	void Prediction::PredictPmoveSingle(pmove_t* pm, int amount)
	{
		Memory::Write(0x537D10, "\xC3");
		for (int i = 0; i < amount; i++)
			PmoveSingle(pm);
		Memory::Write(0x537D10, "\x81");
	}
}
