#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	class Movements : public Module
	{
	public:
		Bind KeyBhop;
		Bind KeyBhopToggle;
		Text BhopText;

		bool UseBhop;
		bool UseBhopToggle;
		bool BhopToggled;

		Movements();
		virtual ~Movements() = default;

		void Menu() override;
		void OnFinishMove(EventPMoveFinish& event) override;
		void OnLoadPosition() override;
		void OnRender() override;

	private:
		void Bhop(playerState_s* ps, usercmd_s* cmd);

		SERIALIZE_POLY(Movements, Module, KeyBhop, KeyBhopToggle, BhopText, UseBhop, UseBhopToggle)
	};
}
