#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class Modules : public Frame
	{
	public:
		Modules();
		virtual ~Modules() = default;

		void OnRender() override;
	};
}
