#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Game prediction.
	/// </summary>
	class API Prediction
	{
	public:
		/// <summary>
		/// Create player movement.
		/// </summary>
		/// <param name="ps">The player state.</param>
		/// <param name="cmd">The user command.</param>
		/// <returns></returns>
		static pmove_t CreatePmove(playerState_s* ps, usercmd_s* cmd);

		/// <summary>
		/// Predict player movement.
		/// </summary>
		/// <param name="pm">The player movement.</param>
		/// <param name="amount">The number of frames to simulate ahead.</param>
		static void PredictPmoveSingle(pmove_t* pm, int amount = 1);
	};
}
