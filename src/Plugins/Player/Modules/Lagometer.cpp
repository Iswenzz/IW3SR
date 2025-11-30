#include "Lagometer.hpp"

int* lagometer = Signature(0x7440D8);
int& lagometerIndex = Signature(0x7442D8);
int* pings = Signature(0x7444DC);
int& pingIndex = Signature(0x7446DC);
int* snapsFlags = Signature(0x7442DC);

namespace IW3SR::Addons
{
	Lagometer::Lagometer() : Module("sr.player.lagometer", "Player", "Lagometer")
	{
		Graph = Plots();
		Graph.SetRect(-55, -140, 48, 48);
		Graph.SetRectAlignment(Horizontal::Right, Vertical::Bottom);

		ColorSnap = { 0, 0, 1, 1 };
		ColorSnapDelay = { 1, 1, 0, 1 };
		ColorSnapFlag = { 0, 1, 0, 1 };
		ColorSnapFlagDrop = { 1, 1, 0, 1 };
		ColorPing = { 1, 0, 0, 1 };

		ShowSnap = true;
		ShowSnapFlag = true;
		ShowPing = true;
	}

	void Lagometer::Initialize()
	{
		Dvar::Set("cg_drawLagometer", 0);
	}

	void Lagometer::Menu()
	{
		ImGui::Checkbox("Snapshot", &ShowSnap);
		ImGui::Checkbox("Snapshot Flags", &ShowSnapFlag);
		ImGui::Checkbox("Ping", &ShowPing);
		Graph.Menu("Graph Options");
	}

	void Lagometer::OnRender()
	{
		const int range = 1000;
		const int snapRange = 667;
		const int pingRange = 500;

		int snap = lagometer[(lagometerIndex - 1) & 0x7F];
		Snaps.Add(snap < 0 ? snapRange + snap : snapRange);
		SnapsDelay.Add(snap > 0 ? snapRange + snap : snapRange);

		Pings.Clear();
		SnapsFlags.Clear();
		SnapsFlagsDrop.Clear();

		int pingSize = Pings.Max();
		for (int i = 0; i < pingSize; i++)
		{
			int index = pingIndex - pingSize + i - 1;
			int snapFlags = snapsFlags[index & 0x7F] & 1;
			int ping = pings[index & 0x7F];

			if (ping > pingRange)
				ping = pingRange;

			Pings.Add(ping < 0 ? pingRange : 0);
			SnapsFlags.Add(snapFlags == 0 ? ping : 0);
			SnapsFlagsDrop.Add(snapFlags != 0 ? ping : 0);
		}

		Graph.Begin();
		if (ImPlot::BeginPlot("##Ping", Graph.RenderSize, ImPlotFlags_NoLegend))
		{
			ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Canvas, ImPlotAxisFlags_Canvas);
			ImPlot::SetupAxisLimits(ImAxis_X1, 0, Snaps.Max(), ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, range, ImGuiCond_Always);

			if (ShowSnap)
			{
				ImPlot::SetNextFillStyle(ColorSnap);
				ImPlot::PlotShaded("Snap", Snaps.Get(), Snaps.Max(), snapRange, 1, 0, 0, Snaps.Offset);

				ImPlot::SetNextFillStyle(ColorSnapDelay);
				ImPlot::PlotShaded("SnapDelay", SnapsDelay.Get(), SnapsDelay.Max(), snapRange, 1, 0, 0,
					SnapsDelay.Offset);
			}
			if (ShowPing)
			{
				ImPlot::SetNextFillStyle(ColorPing);
				ImPlot::PlotShaded("Ping", Pings.Get(), Pings.Max(), -pingRange, 1, 0, 0, Pings.Offset);
			}
			if (ShowSnapFlag)
			{
				ImPlot::SetNextFillStyle(ColorSnapFlag);
				ImPlot::PlotShaded("SnapFlag", SnapsFlags.Get(), SnapsFlags.Max(), -pingRange, 1, 0, 0,
					SnapsFlags.Offset);

				ImPlot::SetNextFillStyle(ColorSnapFlagDrop);
				ImPlot::PlotShaded("SnapFlagDrop", SnapsFlagsDrop.Get(), SnapsFlagsDrop.Max(), -pingRange, 1, 0, 0,
					SnapsFlagsDrop.Offset);
			}
			ImPlot::EndPlot();
		}
		Graph.End();
	}
}
