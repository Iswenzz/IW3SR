#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	enum class StrafeBotMode
	{
		WStrafe,
		StrafeOnly
	};

	class StrafeBot : public Module
	{
	public:
		bool UseLatchSide;
		bool UseReleaseToStop;
		bool UseGroundStrafe;
		int Side;
		StrafeBotMode Mode;

		StrafeBot();
		virtual ~StrafeBot() = default;

		void Menu() override;
		void OnFinishMove(EventPMoveFinish& event) override;

	private:
		bool wasAirborne = false;

		float ComputeOptimalYaw(playerState_s* ps, int side) const;
		int AutoDetectSide(playerState_s* ps, usercmd_s* cmd) const;

		SERIALIZE_POLY(StrafeBot, Module, UseLatchSide, UseReleaseToStop, UseGroundStrafe, Side, Mode)
	};
}
