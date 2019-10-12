//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#include <ADAPT/GPM2/GPMCanvas.h>

using namespace adapt::gpm2;

struct Gaussian
{
	Gaussian(double m, double v) : mean(m), var(v) {}
	double operator()(double x) const
	{
		double d = x - mean;
		return std::exp(-(d * d) / (2 * var)) / std::sqrt(2 * 3.1415926535 * var);
	}
	double mean;
	double var;
};

int main()
{
	//GPMCanvas::SetGnuplotPath("path to gnuplot");

	std::vector<double> x(600, 0);
	std::vector<double> y1(600, 0);
	std::vector<double> y2(600, 0);
	std::vector<double> y3(600, 0);
	Gaussian g1(0, 0.5);
	Gaussian g2(0, 2.0);
	Gaussian g3(-2, 1.0);

	for (int i = 0; i < 600; ++i)
	{
		x[i] = i * 0.02 - 6.;
		y1[i] = g1(x[i]);
		y2[i] = g2(x[i]);
		y3[i] = g3(x[i]);
	}

	/*
	1. PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, Options ...options)
	2. PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& y2, Options ...options)
	the area between the curve and (1)a line given bu baseline option(y=0 by default) (2)y2 curve is filled.

	 * options for FilledCurves
	title          ... title.
	axis           ... set of axes to scale lines. (e.g. plot::axis = "x1y2")
	fillpattern
	filltransparent
	fillcolor
	variable_fillcolor
	bordercolor
	bordertype
	baseline       ... the area between the curve this line is filled. (e.g. plot::baseline = "y=10")
	closed         ... the points are treated as a closed polygon and its inside is filled.
	above          ... the filled area is limited to the above side of baseline or y2 curve.
	below          ... the filled area is limited to the below side of baseline or y2 curve.
	*/

	GPMCanvas2D g("example_filledcurve.png");
	g.SetTitle("example\\_filledcurve");
	g.SetXRange(-4.0, 4.0);
	g.SetXLabel("x");
	g.SetYLabel("y");
	g.PlotFilledCurves(x, y1, plot::title = "mu = 0, sigma^2 = 0.2", plot::fillcolor = "red", plot::filltransparent = 0.4).
		PlotFilledCurves(x, y2, plot::title = "mu = 0, sigma^2 = 1.0", plot::fillcolor = "blue", plot::filltransparent = 0.4).
		PlotFilledCurves(x, y3, plot::title = "mu = -2, sigma^2 = 0.7", plot::fillcolor = "green", plot::filltransparent = 0.4);
	return 0;
}
