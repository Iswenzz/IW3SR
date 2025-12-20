#pragma once
#include "Game/Base.hpp"

#include "PMove.hpp"

namespace IW3SR
{
	class API Player
	{
	public:
		centity_s* ent;
		clientInfo_t* info;

		Player() = default;
		Player(int index);
		~Player() = default;

		bool IsSelf();
		bool IsAlive();
		bool OnGround();

		operator bool() const;

	public:
		static void Initialize();
		static std::array<Ref<Player>, 64>& GetAll();
		static Ref<Player>& Get(int index);
		static Ref<Player>& Self();

	private:
		static inline std::array<Ref<Player>, 64> Players;
	};
}
