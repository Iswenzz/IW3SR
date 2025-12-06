#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class About : public Frame
	{
	public:
		About();
		virtual ~About() = default;

		void OnRender() override;
	};
}
