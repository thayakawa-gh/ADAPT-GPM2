//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#include <ADAPT/GPM2/GPMCanvas.h>

using namespace adapt::gpm2;

double r(double x, double y)
{
	return std::sqrt(x * x + y * y);
};
double potential(double x, double y)
{
	double p1 = 3 / r(x + 3, y);
	double p2 = 1 / r(x - 3, y);
	return p1 - p2;
};
double fieldx(double x, double y)
{
	double f1 = (x - 3) / std::pow(r(x - 3, y), 3);
	double f2 = 3 * (x + 3) / std::pow(r(x + 3, y), 3);
	return f1 - f2;
}
double fieldy(double x, double y)
{
	double f1 = y / std::pow(r(x - 3, y), 3);
	double f2 = 3 * y / std::pow(r(x + 3, y), 3);
	return f1 - f2;
}
int main()
{
	//GPMCanvas::SetGnuplotPath("path to gnuplot");

	adapt::Matrix<double> m(200, 200);
	std::vector<double> xfrom(400), yfrom(400), xlen(400), ylen(400);
	for (int iy = -100; iy < 100; ++iy)
	{
		double y = iy * 0.1;
		for (int ix = -100; ix < 100; ++ix)
		{
			double x = ix * 0.1;
			m[ix + 100][iy + 100] = potential(x, y);
			if (ix % 10 == 0 && iy % 10 == 0)
			{
				int jx = (ix + 100) / 10;
				int jy = (iy + 100) / 10;
				xfrom[jy * 20 + jx] = x;
				yfrom[jy * 20 + jx] = y;
				double xlen_ = fieldx(x, y);
				double ylen_ = fieldy(x, y);
				double len = std::sqrt(xlen_ * xlen_ + ylen_ * ylen_);
				xlen[jy * 20 + jx] = xlen_ / len * 0.8;
				ylen[jy * 20 + jx] = ylen_ / len * 0.8;
			}
		}
	}

	/* options for Colormap
	title          ... title.
	axis           ... set of axes to scale lines. (e.g. plot::axis = "x1y2")
	*/

	/* options for Vectors
	title          ... title.
	axis           ... set of axes to scale lines. (e.g. plot::axis = "x1y2")
	color          ... uniform color.
	variable_color ... different color at each point.
	linetype       ... line type. see GNUPLOT's keyword "linetype"
	linewidth      ... uniform line width.
	arrowhead      ... arrowhead style. (e.g. plot::arrowhead = ArrowHead::heads + ArrowHead::nofilled)
	*/

	GPMCanvasCM g("example_colormap.png");
	g.SetPaletteDefined({ { 0, "cyan" }, { 3.6, "blue" }, { 4., "black" }, { 4.4, "red"}, { 8, "yellow" } });
	g.SetTitle("example\\_colormap");
	g.SetSizeRatio(-1);
	g.SetXLabel("x");
	g.SetYLabel("y");
	g.SetXRange(-10, 10);
	g.SetYRange(-10, 10);
	g.SetCBRange(-5, 5);
	std::pair<double, double> xrange = { -10, 10 };
	std::pair<double, double> yrange = { -10, 10 };
	g.PlotColormap(m, xrange, yrange, plot::title = "notitle").
		PlotVectors(xfrom, yfrom, xlen, ylen, plot::title = "notitle", plot::color = "white");
	return 0;
}
