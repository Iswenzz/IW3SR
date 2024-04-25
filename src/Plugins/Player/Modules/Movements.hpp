#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Movement mode.
	/// </summary>
	enum class MovementMode
	{
		COD4,
		Q3,
		CS
	};

	/// <summary>
	/// Movements features.
	/// </summary>
	class Movements : public Module
	{
	public:
		MovementMode Mode;
		Keyboard BhopKey;
		Text BhopText;

		bool UseBhop;
		bool UseBhopToggle;
		bool UseInterpolateMovers;

		/// <summary>
		/// Create the module.
		/// </summary>
		Movements();
		virtual ~Movements() = default;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void OnMenu() override;

		/// <summary>
		/// Client predict.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnPredict(EventClientPredict& event) override;

		/// <summary>
		/// Walk moving.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnWalkMove(EventPMoveWalk& event) override;

		/// <summary>
		/// Air moving.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnAirMove(EventPMoveAir& event) override;

		/// <summary>
		/// Finish moving.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnFinishMove(EventPMoveFinish& event) override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;

	private:
		/// <summary>
		/// Bunny hop.
		/// </summary>
		/// <param name="cmd">The user command.</param>
		void Bhop(usercmd_s* cmd);

		/// <summary>
		/// Set crash land penalty state.
		/// </summary>
		/// <param name="penalty">Slow down landing.</param>
		void SetCrashLand(bool penalty);

		/// <summary>
		/// Interpolate view angles for mover.
		/// </summary>
		void InterpolateViewForMover();

		SERIALIZE_POLY(Movements, Module, BhopKey, BhopText, UseBhop, UseInterpolateMovers)
	};
}
