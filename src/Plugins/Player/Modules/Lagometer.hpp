#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
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

		Lagometer();
		virtual ~Lagometer() = default;

		void Initialize() override;
		void Menu() override;

		void OnRender() override;

		SERIALIZE_POLY(Lagometer, Module, ColorSnap, ColorSnapDelay, ColorSnapFlag, ColorSnapFlagDrop, ColorPing,
			ShowSnap, ShowSnapFlag, ShowPing, Graph)
	};
}
