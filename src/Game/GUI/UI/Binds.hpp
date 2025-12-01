#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class Binds : public Frame
	{
	public:
		Binds();
		virtual ~Binds() = default;

		void OnRender() override;
	};
}
