//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#include <ADAPT/GPM2/GPMCanvas.h>

using namespace adapt::gpm2;

int main()
{
	GPMCanvas::SetGnuplotPath("path to gnuplot");

	std::string norm = std::to_string(250. / std::sqrt(2 * 3.1415926535));
	std::string equation = norm + "*exp(-x*x/2)";

	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::normal_distribution<> nd(0., 1.);
	std::vector<double> x1(32, 0);
	std::vector<double> y1(32, 0);
	std::vector<double> e1(32);
	for (int i = 0; i < 1000; ++i)
	{
		double x = nd(mt);
		if (x < -4.0 || x >= 4.0) continue;
		++y1[std::floor(x / 0.25) + 16];
	}
	for (int i = 0; i < 32; ++i)
	{
		x1[i] = i * 0.25 - 4. + 0.125;
		e1[i] = std::sqrt(y1[i]);
	}

	GPMCanvas2D g("test_2d.png");
	g.SetTitle("example\\_2d");
	g.SetXRange(-4.0, 4.0);
	g.SetXLabel("x");
	g.SetYLabel("y");
	g.PlotPoints(equation, plot::title = "mean = 0, sigma = 1",
				 plot::style = Style::LINES).
		PlotPoints(x1, y1, plot::xerrorbar = 0.125, plot::yerrorbar = e1,
				   plot::title = "data", plot::color = "black",
				   plot::style = Style::POINTS, plot::pointtype = 7, plot::pointsize = 0.5);
	return 0;
}