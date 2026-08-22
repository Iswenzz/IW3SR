#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	class CEF : public Module
	{
	public:
		Ref<BrowserInstance> Instance;
		bool Interactive = false;

		CEF();
		virtual ~CEF() = default;

		void Initialize() override;
		void Release() override;

		void Menu() override;
		void OnRender() override;

		SERIALIZE_POLY(CEF, Module, Interactive)
	};
}
