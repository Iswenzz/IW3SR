#include "Prediction.hpp"
#include "Engine/Core/Memory/Memory.hpp"
#include "Game/Definitions/Structs.hpp"
#include "Game/System/Dvar.hpp"
#include "PMove.hpp"

#define MASK_PLAYERSOLID (0x02810011)

namespace IW3SR
{
	pmove_t Prediction::CreatePM(playerState_s* ps, usercmd_s* cmd)
	{
		std::unordered_map<int, int> stance = { { 60, 70 }, { 40, 50 }, { 11, 30 } };
		usercmd_s* oldcmd = PMove::GetUserCommand(clients->cmdNumber - 1);
		pmove_t pm{};

		pm.ps = ps;
		pm.cmd = *cmd;
		pm.oldcmd = *oldcmd;

		pm.mins[0] = -15;
		pm.mins[1] = -15;
		pm.mins[2] = 0;
		pm.maxs[0] = 15;
		pm.maxs[1] = 15;
		pm.maxs[2] = stance.find(ps->viewHeightCurrent)->second;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.handler = 1;

		return pm;
	}

	void Prediction::PredictPmoveSingle(pmove_t* pm, int numRep)
	{
		Memory::Write(0x537D10, "\xC3");
		for ([[unused]] int i : std::views::iota(1, numRep))
			PmoveSingle(pm);
		Memory::Write(0x537D10, "\x81");
	}
}
