#include "FPS.hpp"

namespace IW3SR::Addons
{
	FPS::FPS() : Module("sr.player.fps", "Player", "FPS")
	{
		Graph = Plots();

		FrameText = Text("0", FONT_SPACERANGER, -30, 0, 1.4, { 1, 1, 1, 1 });
		FrameText.SetRectAlignment(Horizontal::Right, Vertical::Top);
		FrameText.SetAlignment(Alignment::Center, Alignment::Bottom);

		Value = 0;
		ShowGraph = false;
	}

	void FPS::Menu()
	{
		ImGui::Checkbox("Graph", &ShowGraph);
		FrameText.Menu("Text", true);
		Graph.Menu("Graph Options");
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
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, Values.Max(), ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1000, ImGuiCond_Always);

				ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
				ImPlot::PushStyleColor(ImPlotCol_Line, Math::RGBA(FrameText.Color));
				ImPlot::PlotShaded("FPS", Values.Get(), Values.Max(), -INFINITY, 1, 0, 0, Values.Offset);
				ImPlot::PlotLine("FPS", Values.Get(), Values.Max(), 1, 0, 0, Values.Offset);
				ImPlot::PopStyleColor();

				ImPlot::EndPlot();
			}
			Graph.End();
		}
	}
}
