#include "Player.hpp"

namespace IW3SR
{
	Player::Player(int index)
	{
		if (index < 0 || index >= MAX_PLAYERS)
			return;

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
		return ent && ent->nextState.clientNum == cgs->clientNum;
	}

	bool Player::IsAlive()
	{
		return ent && ent->isAlive;
	}

	bool Player::OnGround()
	{
		return ent && ent->nextState.groundEntityNum != ENTITYNUM_NONE;
	}

	Player::operator bool() const
	{
		return ent != nullptr;
	}

	std::array<Ref<Player>, MAX_PLAYERS>& Player::GetAll()
	{
		return Players;
	}

	Ref<Player>& Player::Get(int index)
	{
		if (index < 0 || index >= MAX_PLAYERS)
			return None;

		return Players[index];
	}

	Ref<Player>& Player::Self()
	{
		return Get(cgs->clientNum);
	}
}
