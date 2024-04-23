#pragma once
#include "Game/Plugin.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Draw lagometer.
	/// </summary>
	class Lagometer : public Module
	{
	public:
		CircularBuffer<int, 48> Snaps;
		CircularBuffer<int, 48> SnapsDelay;
		CircularBuffer<int, 48> SnapsFlags;
		CircularBuffer<int, 48> SnapsFlagsDrop;
		CircularBuffer<int, 48> Pings;
		Plots Graph;

		vec4 ColorSnap;
		vec4 ColorSnapDelay;
		vec4 ColorSnapFlag;
		vec4 ColorSnapFlagDrop;
		vec4 ColorPing;

		bool ShowSnap;
		bool ShowSnapFlag;
		bool ShowPing;

		/// <summary>
		/// Create the module.
		/// </summary>
		Lagometer();
		virtual ~Lagometer() = default;

		/// <summary>
		/// Initialize lagometer.
		/// </summary>
		void Initialize() override;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void OnMenu() override;

		/// <summary>
		/// Render frame.
		/// </summary>
		void OnRender() override;

		SERIALIZE_POLY(Lagometer, Module, ColorSnap, ColorSnapDelay, ColorSnapFlag, ColorSnapFlagDrop, ColorPing,
			ShowSnap, ShowSnapFlag, ShowPing, Graph)
	};
}
