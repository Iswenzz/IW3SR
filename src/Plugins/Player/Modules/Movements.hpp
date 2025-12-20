#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	enum class MovementMode
	{
		COD4,
		Q3,
		CS
	};

	class Movements : public Module
	{
	public:
		Bind KeyBhopToggle;
		Text BhopText;

		bool UseBhop;
		bool UseBhopToggle;
		bool UseInterpolateMovers;
		bool BhopToggled;

		Movements();
		virtual ~Movements() = default;

		void Menu() override;
		void OnPredict(EventClientPredict& event) override;
		void OnWalkMove(EventPMoveWalk& event) override;
		void OnAirMove(EventPMoveAir& event) override;
		void OnGroundTrace(EventPMoveGroundTrace& event) override;
		void OnFinishMove(EventPMoveFinish& event) override;
		void OnLoadPosition() override;
		void OnRender() override;

	private:
		void Bhop(playerState_s* ps, usercmd_s* cmd);
		void SetCrashLand(bool state);
		void InterpolateViewForMover();

		SERIALIZE_POLY(Movements, Module, KeyBhopToggle, BhopText, UseBhop, UseBhopToggle, UseInterpolateMovers)
	};
}
