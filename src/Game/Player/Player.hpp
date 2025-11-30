#pragma once
#include "Game/Base.hpp"

#include "PMove.hpp"

namespace IW3SR
{
	/// <summary>
	/// Player class.
	/// </summary>
	class API Player
	{
	public:
		centity_s* ent;
		clientInfo_t* info;

		/// <summary>
		/// Initialize the player.
		/// </summary>
		Player() = default;
		~Player() = default;

		/// <summary>
		/// Initialize the player.
		/// </summary>
		/// <param name="index">The player index.</param>
		Player(int index);

		/// <summary>
		/// Is player self.
		/// </summary>
		/// <returns></returns>
		bool IsSelf();

		/// <summary>
		/// Is player alive.
		/// </summary>
		/// <returns></returns>
		bool IsAlive();

		/// <summary>
		/// Is player on ground.
		/// </summary>
		/// <returns></returns>
		bool OnGround();

		/// <summary>
		/// Is player in air.
		/// </summary>
		/// <returns></returns>
		bool InAir();

		/// <summary>
		/// Check if the player is active.
		/// </summary>
		operator bool() const;

	public:
		/// <summary>
		/// Get all players.
		/// </summary>
		/// <returns></returns>
		static std::array<Ref<Player>, 64>& GetAll();

		/// <summary>
		/// Get a player.
		/// </summary>
		/// <param name="index">The player index.</param>
		/// <returns></returns>
		static Ref<Player>& Get(int index);

		/// <summary>
		/// Get self.
		/// </summary>
		/// <returns></returns>
		static Ref<Player>& Self();

	private:
		static inline std::array<Ref<Player>, 64> Players;
	};
}
