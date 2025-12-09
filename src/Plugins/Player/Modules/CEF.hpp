#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	class CEF : public Module
	{
	public:
		CEF();
		virtual ~CEF() = default;

		void Initialize() override;
		void Release() override;

		void Menu() override;
		void OnRender() override;
	};
}
