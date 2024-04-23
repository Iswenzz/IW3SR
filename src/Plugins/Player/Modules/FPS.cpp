#include "FPS.hpp"

namespace IW3SR::Addons
{
	FPS::FPS() : Module("sr.player.fps", "Player", "FPS")
	{
		Graph = Plots();

		FrameText = Text("0", FONT_SPACERANGER, -30, 0, 1.4, { 1, 1, 1, 1 });
		FrameText.SetRectAlignment(HORIZONTAL_RIGHT, VERTICAL_TOP);
		FrameText.SetAlignment(ALIGN_CENTER, ALIGN_BOTTOM);

		Value = 0;
		ShowGraph = false;
	}

	void FPS::OnMenu()
	{
		ImGui::Checkbox("Display Graph", &ShowGraph);
		FrameText.Menu("Text", true);
		Graph.Menu("Graph");
	}

	void FPS::OnRender()
	{
		Value = Dvar::Get<int>("com_maxfps");
		Values.Add(Value);

		FrameText.Value = std::to_string(Value);
		FrameText.Render();

		if (ShowGraph)
		{
			Graph.Begin();
			if (ImPlot::BeginPlot("##FPS", Graph.RenderSize))
			{
				ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Canvas, ImPlotAxisFlags_Canvas);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, Values.Size(), ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1000, ImGuiCond_Always);

				ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
				ImPlot::PushStyleColor(ImPlotCol_Line, FrameText.Color.RGBA());
				ImPlot::PlotShaded("FPS", Values.Get(), Values.Size(), -INFINITY, 1, 0, 0, Values.Offset);
				ImPlot::PlotLine("FPS", Values.Get(), Values.Size(), 1, 0, 0, Values.Offset);
				ImPlot::PopStyleColor();

				ImPlot::EndPlot();
			}
			Graph.End();
		}
	}
}
