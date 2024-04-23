#include "Velocity.hpp"

namespace IW3SR::Addons
{
	Velocity::Velocity() : Module("sr.player.velocity", "Player", "Velocity")
	{
		Graph = Plots();

		VelocityText = Text("0", FONT_SPACERANGER, 0, 2, 1.4, { 0, 1, 1, 1 });
		VelocityText.SetRectAlignment(HORIZONTAL_CENTER, VERTICAL_TOP);
		VelocityText.SetAlignment(ALIGN_CENTER, ALIGN_BOTTOM);

		AverageText = Text("0", FONT_SPACERANGER, 50, 2, 1.4, { 1, 1, 0, 1 });
		AverageText.SetRectAlignment(HORIZONTAL_CENTER, VERTICAL_TOP);
		AverageText.SetAlignment(ALIGN_CENTER, ALIGN_BOTTOM);

		MaxText = Text("0", FONT_SPACERANGER, 100, 2, 1.4, { 1, 0, 0, 1 });
		MaxText.SetRectAlignment(HORIZONTAL_CENTER, VERTICAL_TOP);
		MaxText.SetAlignment(ALIGN_CENTER, ALIGN_BOTTOM);

		GroundText = Text("0", FONT_SPACERANGER, -50, 2, 1.4, { 0, 1, 0, 1 });
		GroundText.SetRectAlignment(HORIZONTAL_CENTER, VERTICAL_TOP);
		GroundText.SetAlignment(ALIGN_CENTER, ALIGN_BOTTOM);

		ShowVelocity = true;
		ShowAverage = false;
		ShowMax = false;
		ShowGround = false;
		ShowGroundTime = true;
		ShowGraph = false;

		KeyReset = Keyboard(Key_R);
	}

	void Velocity::Compute()
	{
		static bool prevOnGround = true;
		int prevVelocity = vec2(pmove->ps->oldVelocity).Length();

		bool onGround = PMove::OnGround();
		bool landed = onGround && !prevOnGround;

		if (landed)
		{
			Averages.Add(prevVelocity);
			Average = Averages.Average();
			GroundAverages.Add(GroundTime);
			GroundAverage = GroundAverages.Average();
			GroundTime = 0;
		}
		if (onGround)
			GroundTime += UI::Get().DeltaTimeMS();

		Value = vec2(pmove->ps->velocity).Length();
		BufferValues.Add(Value);
		BufferAverages.Add(Average);

		Max = Value > Max ? Value : Max;
		BufferMaxs.Add(Max);

		Ground = ShowGroundTime ? GroundTime : GroundAverage;
		Ground = Ground < 1000 ? Ground : 1000;
		BufferGrounds.Add(Ground < Max ? Ground : Max);

		prevOnGround = onGround;
	}

	void Velocity::OnMenu()
	{
		ImGui::Checkbox("Velocity", &ShowVelocity);
		ImGui::Checkbox("Average", &ShowAverage);
		ImGui::Checkbox("Max", &ShowMax);

		if (ShowMax)
		{
			ImGui::SameLine();
			ImGui::Keybind("Reset", &KeyReset.Key);
		}
		ImGui::Checkbox("Ground", &ShowGround);

		if (ShowGround)
		{
			ImGui::SameLine();
			ImGui::Checkbox("Time", &ShowGroundTime);
		}
		ImGui::Checkbox("Display Graph", &ShowGraph);

		VelocityText.Menu("Velocity");
		AverageText.Menu("Average");
		MaxText.Menu("Max");
		GroundText.Menu("Ground");
		Graph.Menu("Graph");
	}

	void Velocity::OnRender()
	{
		Compute();

		VelocityText.Value = std::to_string(Value);
		AverageText.Value = std::to_string(Average);
		MaxText.Value = std::to_string(Max);
		GroundText.Value = std::to_string(Ground);

		if (ShowVelocity)
			VelocityText.Render();
		if (ShowAverage)
			AverageText.Render();
		if (ShowMax)
			MaxText.Render();
		if (ShowGround)
			GroundText.Render();

		if (KeyReset.IsPressed())
		{
			Average = 0;
			Max = 0;
			GroundAverage = 0;
			GroundTime = 0;

			BufferAverages.Clear();
			BufferMaxs.Clear();
			BufferGrounds.Clear();
		}
		if (ShowGraph)
		{
			Graph.Begin();
			if (ImPlot::BeginPlot("##Velocity", Graph.RenderSize))
			{
				ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Canvas, ImPlotAxisFlags_Canvas);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, BufferValues.Size(), ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, Max * 1.5, ImGuiCond_Always);

				if (ShowVelocity)
				{
					ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
					ImPlot::PushStyleColor(ImPlotCol_Line, VelocityText.Color.RGBA());
					ImPlot::PlotShaded("Velocity", BufferValues.Get(), BufferValues.Size(), -INFINITY, 1, 0, 0,
						BufferValues.Offset);
					ImPlot::PlotLine("Velocity", BufferValues.Get(), BufferValues.Size(), 1, 0, 0, BufferValues.Offset);
					ImPlot::PopStyleColor();
				}
				if (ShowAverage)
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, AverageText.Color.RGBA());
					ImPlot::PlotLine("Average", BufferAverages.Get(), BufferAverages.Size(), 1, 0, 0,
						BufferAverages.Offset);
					ImPlot::PopStyleColor();
				}
				if (ShowMax)
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, MaxText.Color.RGBA());
					ImPlot::PlotLine("Max", BufferMaxs.Get(), BufferMaxs.Size(), 1, 0, 0, BufferMaxs.Offset);
					ImPlot::PopStyleColor();
				}
				if (ShowGround)
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, GroundText.Color.RGBA());
					ImPlot::PlotLine("Ground", BufferGrounds.Get(), BufferGrounds.Size(), 1, 0, 0,
						BufferGrounds.Offset);
					ImPlot::PopStyleColor();
				}
				ImPlot::EndPlot();
			}
			Graph.End();
		}
	}
}
