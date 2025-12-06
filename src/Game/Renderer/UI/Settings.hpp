#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class Settings : public Frame
	{
	public:
		Settings();
		virtual ~Settings() = default;

		void OnRender() override;
	};
}
