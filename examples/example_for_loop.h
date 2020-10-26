#ifndef EXAMPLE_FORLOOP_H
#define EXAMPLE_FORLOOP_H

#include <ADAPT/GPM2/GPMCanvas.h>

using namespace adapt::gpm2;

int example_for_loop(const std::string output_filename = "example_for_loop.png", const bool enable_in_memory_data_transfer = false)
{
	GPMCanvas2D g(output_filename);
	g.ShowCommands(true);
	g.EnableInMemoryDataTransfer(enable_in_memory_data_transfer); // Enable or disable datablock feature of gnuplot

	auto buf = g.GetBuffer();
	for (int i = 0; i < 5; ++i)
	{
		std::string eq = std::to_string(i) + "*x";
		buf = buf.PlotPoints(eq, plot::title = eq, plot::style = Style::lines);
	}
	buf.Flush();
	return 0;
}

#endif