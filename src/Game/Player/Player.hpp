#pragma once
#include "PMove.hpp"

namespace IW3SR
{
	constexpr int MAX_PLAYERS = 64;

	class API Player
	{
	public:
		centity_s* ent = nullptr;
		clientInfo_t* info = nullptr;

		Player() = default;
		Player(int index);
		~Player() = default;

		bool IsSelf();
		bool IsAlive();
		bool OnGround();

		operator bool() const;

	public:
		static void Initialize();
		static std::array<Ref<Player>, MAX_PLAYERS>& GetAll();
		static Ref<Player>& Get(int index);
		static Ref<Player>& Self();

	private:
		static inline std::array<Ref<Player>, MAX_PLAYERS> Players;
		static inline Ref<Player> None;
	};
}
