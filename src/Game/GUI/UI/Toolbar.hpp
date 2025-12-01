#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class Toolbar : public Frame
	{
	public:
		bool IsReloading = false;
		bool IsDebug = false;

		Toolbar();
		virtual ~Toolbar() = default;

		void OnRender() override;

	private:
		void Reload();
		void Compile();
	};
}
