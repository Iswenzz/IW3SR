#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Prediction
	{
	public:
		static pmove_t CreatePmove(playerState_s* ps, usercmd_s* cmd);
		static void PredictPmoveSingle(pmove_t* pm, int amount = 1);
	};
}
