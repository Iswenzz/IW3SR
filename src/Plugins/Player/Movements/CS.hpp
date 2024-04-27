#pragma once
// https://github.com/xoxor4d/iw3xo-dev/blob/master/src/components/modules/movement.cpp
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// CS Movements.
	/// </summary>
	class CS
	{
	public:
		/// <summary>
		/// Air move.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		static void AirMove(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Air accelerate.
		/// </summary>
		/// <param name="wishdir">The wish dir.</param>
		/// <param name="wishspeed">The wish speed.</param>
		/// <param name="ps">The player state.</param>
		/// <param name="pml">Player move library.</param>
		static void AirAccelerate(vec3_t wishdir, float wishspeed, playerState_s* ps, pml_t* pml);

		/// <summary>
		/// Try player move.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		static void TryPlayerMove(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Clip velocity.
		/// </summary>
		/// <param name="in">In velocity.</param>
		/// <param name="normal">The normal vector.</param>
		/// <param name="out">Out velocity.</param>
		/// <param name="overbounce">Overbounce value.</param>
		static void ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);
	};
}
