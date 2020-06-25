#ifndef EXAMPLE_FILLEDCURVE_H
#define EXAMPLE_FILLEDCURVE_H

#include <ADAPT/GPM2/GPMCanvas.h>

using namespace adapt::gpm2; 

struct ChiSquare
{
	ChiSquare(double k) : k(k) {}
	double operator()(double x) const
	{
		return std::pow(x, k / 2 - 1) * std::exp(-x / 2) / (std::pow(2, k / 2) * tgamma(k / 2));
	}
	double k;
};

int example_filledcurve()
{
	//GPMCanvas::SetGnuplotPath("path to gnuplot");

	std::vector<double> x(401, 0);
	std::vector<double> y1(401, 0);
	std::vector<double> y2(401, 0);
	std::vector<double> y3(401, 0);
	ChiSquare c1(1);
	ChiSquare c2(2);
	ChiSquare c3(3);

	for (int i = 0; i <= 400; ++i)
	{
		x[i] = i * 0.02;
		y1[i] = c1(x[i]);
		y2[i] = c2(x[i]);
		y3[i] = c3(x[i]);
	}
	y1[0] = 10;

	/*
	1. PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, Options ...options)
	2. PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& y2, Options ...options)
	the area between the curve and (1)a line given by baseline option(y=0 by default) (2)y2 curve is filled.
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
	g.ShowCommands(true);
	g.SetTitle("example\\_filledcurve");
	g.SetXRange(0, 8.0);
	g.SetYRange(0, 1.0);
	g.SetXLabel("x");
	g.SetYLabel("y");
	g.PlotFilledCurves(x, y1, plot::title = "k = 1", plot::fillcolor = "red", plot::filltransparent = 0.4).
		PlotFilledCurves(x, y2, plot::title = "k = 2", plot::fillcolor = "blue", plot::filltransparent = 0.4).
		PlotFilledCurves(x, y3, plot::title = "k = 3", plot::fillcolor = "green", plot::filltransparent = 0.4);
	return 0;
}

#endif