#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Draw current velocity.
	/// </summary>
	class Velocity : public Module
	{
	public:
		int Value = 0;
		int Average = 0;
		int Max = 0;
		int Ground = 0;
		int GroundAverage = 0;
		int GroundTime = 0;

		CircularBuffer<int, 1000> Averages;
		CircularBuffer<int, 1000> GroundAverages;

		CircularBuffer<int, 1000> BufferValues;
		CircularBuffer<int, 1000> BufferAverages;
		CircularBuffer<int, 1000> BufferMaxs;
		CircularBuffer<int, 1000> BufferGrounds;

		Text VelocityText;
		Text AverageText;
		Text MaxText;
		Text GroundText;
		Plots Graph;

		Bind KeyReset;
		bool ShowVelocity;
		bool ShowAverage;
		bool ShowMax;
		bool ShowGround;
		bool ShowGroundTime;
		bool ShowGraph;

		/// <summary>
		/// Create the module.
		/// </summary>
		Velocity();
		virtual ~Velocity() = default;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void Menu() override;

		/// <summary>
		/// Client spawn.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnSpawn(EventClientSpawn& event) override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;

	private:
		/// <summary>
		/// Compute values.
		/// </summary>
		void Compute();

		/// <summary>
		/// Reset values.
		/// </summary>
		void Reset();

		SERIALIZE_POLY(Velocity, Module, VelocityText, AverageText, MaxText, GroundText, Graph, KeyReset, ShowVelocity,
			ShowAverage, ShowMax, ShowGround, ShowGraph)
	};
}
