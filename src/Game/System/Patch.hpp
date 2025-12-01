#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class Patch
	{
	public:
		static void Initialize();

	private:
		static void Definitions();
		static void Renderer();
		static void Database();
		static void Player();
		static void System();
		static void CoD4X();
		static void Hook();
	};
}
