#include "Player.hpp"

namespace IW3SR
{
	Player::Player(int index)
	{
		ent = &cg_entities[index];
		info = &cgs->bgs.clientinfo[index];
	}

	void Player::Initialize()
	{
		auto& players = GetAll();
		for (int i = 0; i < players.size(); i++)
			players[i] = CreateRef<Player>(i);
	}

	bool Player::IsSelf()
	{
		return ent->nextState.clientNum == cgs->clientNum;
	}

	bool Player::IsAlive()
	{
		return ent->isAlive;
	}

	bool Player::OnGround()
	{
		return ent->nextState.groundEntityNum != ENTITYNUM_NONE;
	}

	bool Player::InAir()
	{
		return ent->nextState.groundEntityNum == ENTITYNUM_NONE;
	}

	Player::operator bool() const
	{
		return ent;
	}

	std::array<Ref<Player>, 64>& Player::GetAll()
	{
		return Players;
	}

	Ref<Player>& Player::Get(int index)
	{
		return Players[index];
	}

	Ref<Player>& Player::Self()
	{
		return Players[cgs->clientNum];
	}
}
