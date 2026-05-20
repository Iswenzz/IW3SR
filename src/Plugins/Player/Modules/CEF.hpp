#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	class CEF : public Module
	{
	public:
		Ref<BrowserInstance> Instance;
		bool Open = false;

		CEF();
		virtual ~CEF() = default;

		void Initialize() override;
		void Release() override;

		void OnExecuteCommand(EventClientCommand& event) override;
		void OnRender() override;
	};
}
