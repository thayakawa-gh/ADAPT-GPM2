#ifndef EXAMPLE_2D_H
#define EXAMPLE_2D_H

#include <ADAPT/GPM2/GPMCanvas.h>
#include <random>

using namespace adapt::gpm2;

int example_2d(const std::string output_filename = "example_2d.png", const bool enable_in_memory_data_transfer = false)
{
	std::string norm = std::to_string(250. / std::sqrt(2 * 3.1415926535));
	std::string equation = norm + "*exp(-x*x/2)";

	std::mt19937_64 mt(0);
	std::normal_distribution<> nd(0., 1.);
	std::vector<double> e1(32);
	std::vector<double> x1(32, 0);
	std::vector<double> y1(32, 0);
	for (int i = 0; i < 1000; ++i)
	{
		double x = nd(mt);
		if (x < -4.0 || x >= 4.0) continue;
		++y1[static_cast<size_t>(std::floor(x / 0.25) + 16)];
	}
	for (int i = 0; i < 32; ++i)
	{
		x1[i] = i * 0.25 - 4. + 0.125;
		e1[i] = std::sqrt(y1[i]);
	}

	/* options for PlotPoints(...)
	title          ... title.
	axis           ... set of axes to scale lines. (e.g. plot::axis = "x1y2")
	color          ... uniform color.
	variable_color ... different colors at each point.
	linetype       ... line type. see GNUPLOT's keyword "linetype"
	linewidth      ... uniform line width.
	style          ... displaying style.
	pointtype      ... each point is drawn as the symbol specified by this option.
	pointsize      ... uniform point size.
	variable_size  ... different sizes at each point.
	smooth         ... interpolation and approximation by some routines.
	xerrorbar      ... xerrorbar. This is available only if you specify Style::lines, points or boxes option.
	yerrorbar      ... yerrorbar. This is available only if you specify Style::lines, points or boxes option.

	//The following options are available only with Style::boxes or Style::steps
	fillpattern    ... fill with a colored pattern.
	fillsolid      ... fill with a solid color specified by fillcolor with density [ 0.0, 1.0 ].
	filltransparent... make the filled area transparent to the background color.
	fillcolor      ... specify the color of filled area.
	variable_fillcolor ...
	bordercolor    ... specify the color of the border.
	bordertype     ... specify the type of the border.
	*/
	{
		GPMCanvas2D g(output_filename);
		g.ShowCommands(true);
		g.EnableInMemoryDataTransfer(enable_in_memory_data_transfer); // Enable or disable datablock feature of gnuplot
		g.SetTitle("example\\_2d");
		g.SetXRange(-4.0, 4.0);
		g.SetXLabel("x");
		g.SetYLabel("y");
		g.PlotPoints(equation, plot::title = "mu = 0, sigma = 1",
					 plot::style = Style::lines).
			PlotPoints(x1, y1, plot::xerrorbar = 0.125, plot::yerrorbar = e1,
					   plot::title = "data", plot::color = "black",
					   plot::style = Style::points, plot::pointtype = 7, plot::pointsize = 0.5);
	}
	if (!enable_in_memory_data_transfer)
	{
		GPMCanvas2D g(output_filename + ".fileplot.png");
		g.ShowCommands(true);
		g.SetTitle("example\\_2d");
		g.SetXRange(-4.0, 4.0);
		g.SetXLabel("x");
		g.SetYLabel("y");
		g.PlotPoints(equation, plot::title = "mu = 0, sigma = 1",
					 plot::style = Style::lines).
			PlotPoints(output_filename + ".tmp1.txt", "1", "2", plot::xerrorbar = 0.125, plot::yerrorbar = "3",
					   plot::title = "data", plot::color = "black",
					   plot::style = Style::points, plot::pointtype = 7, plot::pointsize = 0.5);
	}
	return 0;
}

#endif