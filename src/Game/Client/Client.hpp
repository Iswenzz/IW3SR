#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Client class.
	/// </summary>
	class Client
	{
	public:
		/// <summary>
		/// Initialize client.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Client connect.
		/// </summary>
		static void Connect();

		/// <summary>
		/// Client disconnect.
		/// </summary>
		/// <param name="localClientNum">Local client num.</param>
		static void Disconnect(int localClientNum);

		/// <summary>
		/// Client respawn.
		/// </summary>
		/// <param name="localClientNum">Local client num.</param>
		static void Respawn(int localClientNum);

		/// <summary>
		/// Predict player state.
		/// </summary>
		/// <param name="localClientNum">Local client num.</param>
		static void Predict(int localClientNum);
	};
}
