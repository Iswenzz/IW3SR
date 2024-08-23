#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Prediction
	{
	public:
		/// <summary>
		/// Create PMove.
		/// </summary>
		/// <param name="ps">The player state.</param>
		/// <param name="cmd">The user command.</param>
		/// <returns></returns>
		static pmove_t CreatePM(playerState_s* ps, usercmd_s* cmd);

		/// <summary>
		/// Predict next frame pmove.
		/// </summary>
		/// <param name="pm">The player movement.</param>
		/// <param name="numRep">The number of frames to simulate ahead.</param>
		static void PredictPmoveSingle(pmove_t* pm, int numRep = 1);
	};
}
