#pragma once
#include "Player/Base.hpp"

namespace IW3SR::Addons
{
	enum class MovementMode
	{
		COD4,
		Q3,
		Q3CPM,
		CS,
	};

	class Movements : public Module
	{
	public:
		Bind KeyBhop;
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
		void SetHardLanding(bool state);
		void InterpolateViewForMover();
		MovementMode GetMovementMode();

		SERIALIZE_POLY(Movements, Module, KeyBhop, KeyBhopToggle, BhopText, UseBhop, UseBhopToggle,
			UseInterpolateMovers)
	};
}
