#pragma once
#include "Game/Base.hpp"

namespace IW3SR::UC
{
	class Demo : public Frame
	{
	public:
		Demo();
		virtual ~Demo() = default;

		void OnRender() override;

	private:
		std::string Output;

		void DrawCaptureTab();
		void DrawAssetsTab();
	};
}
