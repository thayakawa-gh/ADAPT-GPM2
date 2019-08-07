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

	std::vector<double> x1(100);
	std::vector<double> y1(100);
	double mean = 0;
	double sigma = 1;
	double norm = 250. / std::sqrt(2 * 3.1415926535 * sigma * sigma);
	for (int i = -50; i < 50; ++i)
	{
		x1[i + 50] = (double)i / 10.;
		y1[i + 50] = norm * std::exp(-x1[i + 50] * x1[i + 50] / (2 * sigma * sigma));
	}

	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::normal_distribution<> nd(0., 1.);
	std::vector<double> x2(32, 0);
	std::vector<double> y2(32, 0);
	std::vector<double> e2(32);
	for (int i = 0; i < 1000; ++i)
	{
		double x = nd(mt);
		if (x < -4.0 || x >= 4.0) continue;
		++y2[std::floor(x / 0.25) + 16];
	}
	for (int i = 0; i < 32; ++i)
	{
		x2[i] = i * 0.25 - 4. + 0.125;
		e2[i] = std::sqrt(y2[i]);
	}

	GPMCanvas2D g("example_2d.png");
	g.SetTitle("example\\_2d");
	g.SetXRange(-4.0, 4.0);
	g.SetXLabel("x");
	g.SetYLabel("y");
	g.PlotPoints(x1, y1, plot::title = "mean = 0, sigma = 1",
				 plot::style = plot::LINES).
		PlotPoints(x2, y2, plot::xerrorbar = 0.125, plot::yerrorbar = e2,
				   plot::title = "data", plot::color = "black",
				   plot::style = plot::POINTS, plot::pointtype = 7, plot::pointsize = 0.5);
	return 0;
}
