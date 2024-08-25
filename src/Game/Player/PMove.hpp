#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Player movement.
	/// </summary>
	/// <remarks>PMove is shared with self and spectator client.</remarks>
	class API PMove
	{
	public:
		/// <summary>
		/// Get the user command.
		/// </summary>
		/// <param name="cmdNumber">The command number.</param>
		/// <returns></returns>
		static usercmd_s* GetUserCommand(int cmdNumber);

		/// <summary>
		/// Get the eye position.
		/// </summary>
		/// <returns></returns>
		static vec3 GetEyePos();

		/// <summary>
		/// Finish moving.
		/// </summary>
		/// <param name="cmd">The user command.</param>
		static void FinishMove(usercmd_s* cmd);

		/// <summary>
		/// Walk moving.
		/// </summary>
		/// <param name="pm">The player movement.</param>
		/// <param name="pml">The player movement library.</param>
		static void WalkMove(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Air moving.
		/// </summary>
		/// <param name="pm">The player movement.</param>
		/// <param name="pml">The player movement library.</param>
		static void AirMove(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Ground trace.
		/// </summary>
		/// <param name="pm">The player movement.</param>
		/// <param name="pml">The player movement library.</param>
		static void GroundTrace(pmove_t* pm, pml_t* pml);

		/// <summary>
		/// Set yaw.
		/// </summary>
		/// <param name="cmd">The user command.</param>
		/// <param name="target">The target angles.</param>
		static void SetYaw(usercmd_s* cmd, const vec3& target);

		/// <summary>
		/// Set pitch.
		/// </summary>
		/// <param name="cmd">The user command.</param>
		/// <param name="target">The target angles.</param>
		static void SetPitch(usercmd_s* cmd, const vec3& target);

		/// <summary>
		/// Set roll.
		/// </summary>
		/// <param name="cmd">The user command.</param>
		/// <param name="target">The target angles.</param>
		static void SetRoll(usercmd_s* cmd, const vec3& target);

		/// <summary>
		/// Set yaw pitch roll.
		/// </summary>
		/// <param name="cmd">The user command.</param>
		/// <param name="target">The target angles.</param>
		static void SetAngles(usercmd_s* cmd, const vec3& target);

		/// <summary>
		/// Disable sprint.
		/// </summary>
		/// <param name="ps">Player state.</param>
		static void DisableSprint(playerState_s* ps);

		/// <summary>
		/// Is player on ground.
		/// </summary>
		/// <returns></returns>
		static bool OnGround();

		/// <summary>
		/// Is player in air.
		/// </summary>
		/// <returns></returns>
		static bool InAir();
	};
}
