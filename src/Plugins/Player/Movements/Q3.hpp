#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Q3 Movements.
	/// </summary>
	class Q3
	{
	public:
		/// <summary>
		/// Walk move.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		static void WalkMove(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Air move.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		static void AirMove(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Air control.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		/// <param name="wishdir_b">The wish dir.</param>
		/// <param name="wishspeed_b">The wish speed.</param>
		static void AirControl(pmove_t* pm, pml_t* pml, vec3_t wishdir_b, float wishspeed_b);

		/// <summary>
		/// Accelerate.
		/// </summary>
		/// <param name="ps">Player state.</param>
		/// <param name="pml">Player move library.</param>
		/// <param name="wishdir_b">The wish dir.</param>
		/// <param name="wishspeed_b">The wish speed.</param>
		/// <param name="accel_b">The accel.</param>
		static void Accelerate(playerState_s* ps, pml_t* pml, vec3_t wishdir_b, float wishspeed_b, float accel_b);

		/// <summary>
		/// Accelerate walk.
		/// </summary>
		/// <param name="wishdir">The wish dir.</param>
		/// <param name="pml">Player move library.</param>
		/// <param name="ps">Player state.</param>
		/// <param name="wishspeed">The wish speed.</param>
		/// <param name="accel">The accel.</param>
		static void AccelerateWalk(float* wishdir, pml_t* pml, playerState_s* ps, float wishspeed, float accel);

		/// <summary>
		/// Jump check.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		static bool JumpCheck(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Friction.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		static void Friction(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Clip velocity.
		/// </summary>
		/// <param name="in">In velocity.</param>
		/// <param name="normal">Normal vector.</param>
		/// <param name="out">Out velocity.</param>
		/// <param name="overbounce">Overbounce value.</param>
		static void ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);

		/// <summary>
		/// Slide move.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		/// <param name="gravity">Use gravity.</param>
		/// <returns></returns>
		static bool SlideMove(pmove_t* pm, pml_t* pml, bool gravity);

		/// <summary>
		/// Step slide move.
		/// </summary>
		/// <param name="pm">Player move.</param>
		/// <param name="pml">Player move library.</param>
		/// <param name="gravity">Use gravity.</param>
		static void StepSlideMove(pmove_t* pm, pml_t* pml, bool gravity);

		/// <summary>
		/// Scale the user command.
		/// </summary>
		/// <param name="ps">Player state.</param>
		/// <param name="cmd">The user command.</param>
		/// <returns></returns>
		static float CmdScale(playerState_s* ps, usercmd_s* cmd);

		/// <summary>
		/// Set movement direction.
		/// </summary>
		/// <param name="pm">Player move.</param>
		static void SetMovementDir(pmove_t* pm);
	};
}
