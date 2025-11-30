#include "Velocity.hpp"

namespace IW3SR::Addons
{
	Velocity::Velocity() : Module("sr.player.velocity", "Player", "Velocity")
	{
		Graph = Plots();

		VelocityText = Text("0", FONT_SPACERANGER, 0, 2, 1.4, { 0, 1, 1, 1 });
		VelocityText.SetRectAlignment(Horizontal::Center, Vertical::Top);
		VelocityText.SetAlignment(Alignment::Center, Alignment::Bottom);

		AverageText = Text("0", FONT_SPACERANGER, 50, 2, 1.4, { 1, 1, 0, 1 });
		AverageText.SetRectAlignment(Horizontal::Center, Vertical::Top);
		AverageText.SetAlignment(Alignment::Center, Alignment::Bottom);

		MaxText = Text("0", FONT_SPACERANGER, 100, 2, 1.4, { 1, 0, 0, 1 });
		MaxText.SetRectAlignment(Horizontal::Center, Vertical::Top);
		MaxText.SetAlignment(Alignment::Center, Alignment::Bottom);

		GroundText = Text("0", FONT_SPACERANGER, -50, 2, 1.4, { 0, 1, 0, 1 });
		GroundText.SetRectAlignment(Horizontal::Center, Vertical::Top);
		GroundText.SetAlignment(Alignment::Center, Alignment::Bottom);

		ShowVelocity = true;
		ShowAverage = false;
		ShowMax = false;
		ShowGround = false;
		ShowGroundTime = true;
		ShowGraph = false;

		KeyReset = Bind(Key_R);
	}

	void Velocity::Menu()
	{
		ImGui::Checkbox("Velocity", &ShowVelocity);
		ImGui::Checkbox("Average", &ShowAverage);
		ImGui::Checkbox("Max", &ShowMax);

		if (ShowMax)
		{
			ImGui::SameLine();
			ImGui::Keybind("Reset", &KeyReset.Input);
		}
		ImGui::Checkbox("Ground", &ShowGround);

		if (ShowGround)
		{
			ImGui::SameLine();
			ImGui::Checkbox("Time", &ShowGroundTime);
		}
		ImGui::Checkbox("Graph", &ShowGraph);

		VelocityText.Menu("Velocity Options");
		AverageText.Menu("Average Options");
		MaxText.Menu("Max Options");
		GroundText.Menu("Ground Options");
		Graph.Menu("Graph Options");
	}

	void Velocity::Compute()
	{
		static bool prevOnGround = true;
		int prevVelocity = glm::length(vec2(pmove->ps->oldVelocity));

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
			GroundTime += UI::DeltaTimeMS();

		Value = glm::length(vec2(pmove->ps->velocity));
		BufferValues.Add(Value);
		BufferAverages.Add(Average);

		Max = Value > Max ? Value : Max;
		BufferMaxs.Add(Max);

		Ground = ShowGroundTime ? GroundTime : GroundAverage;
		Ground = Ground < 1000 ? Ground : 1000;
		BufferGrounds.Add(Ground < Max ? Ground : Max);

		prevOnGround = onGround;
	}

	void Velocity::Reset()
	{
		Average = 0;
		Max = 0;
		GroundAverage = 0;
		GroundTime = 0;

		Averages.Clear();
		GroundAverages.Clear();

		BufferAverages.Clear();
		BufferMaxs.Clear();
		BufferGrounds.Clear();
	}

	void Velocity::OnSpawn(EventClientSpawn& event)
	{
		Reset();
	}

	void Velocity::OnRender()
	{
		Compute();

		if (KeyReset.IsPressed())
			Reset();

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

		if (ShowGraph)
		{
			Graph.Begin();
			if (ImPlot::BeginPlot("##Velocity", Graph.RenderSize))
			{
				ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Canvas, ImPlotAxisFlags_Canvas);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, BufferValues.Max(), ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, Max * 1.5, ImGuiCond_Always);

				if (ShowVelocity)
				{
					ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
					ImPlot::PushStyleColor(ImPlotCol_Line, Math::RGBA(VelocityText.Color));
					ImPlot::PlotShaded("Velocity", BufferValues.Get(), BufferValues.Max(), -INFINITY, 1, 0, 0,
						BufferValues.Offset);
					ImPlot::PlotLine("Velocity", BufferValues.Get(), BufferValues.Max(), 1, 0, 0, BufferValues.Offset);
					ImPlot::PopStyleColor();
				}
				if (ShowAverage)
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, Math::RGBA(AverageText.Color));
					ImPlot::PlotLine("Average", BufferAverages.Get(), BufferAverages.Max(), 1, 0, 0,
						BufferAverages.Offset);
					ImPlot::PopStyleColor();
				}
				if (ShowMax)
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, Math::RGBA(MaxText.Color));
					ImPlot::PlotLine("Max", BufferMaxs.Get(), BufferMaxs.Max(), 1, 0, 0, BufferMaxs.Offset);
					ImPlot::PopStyleColor();
				}
				if (ShowGround)
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, Math::RGBA(GroundText.Color));
					ImPlot::PlotLine("Ground", BufferGrounds.Get(), BufferGrounds.Max(), 1, 0, 0, BufferGrounds.Offset);
					ImPlot::PopStyleColor();
				}
				ImPlot::EndPlot();
			}
			Graph.End();
		}
	}
}
