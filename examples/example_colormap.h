#ifndef EXAMPLE_COLORMAP_H
#define EXAMPLE_COLORMAP_H

#include <ADAPT/GPM2/GPMCanvas.h>
#include <thread>

using namespace adapt::gpm2;

double r(double x, double y)
{
	return std::sqrt(x * x + y * y);
};
double potential(double x, double y)
{
	double r1 = r(x - 3, y);
	double r2 = r(x + 3, y);
	if (r1 == 0.) r1 = 0.000001;
	if (r2 == 0.) r2 = 0.000001;
	double p1 = 1 / r1;
	double p2 = 3 / r2;
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
int example_colormap(const std::string output_filename = "example_colormap.png", const bool enable_in_memory_data_transfer = false)
{
	adapt::Matrix<double> m(100, 100);
	std::pair<double, double> xrange = { -9.9, 9.9 };
	std::pair<double, double> yrange = { -9.9, 9.9 };
	for (int iy = -50; iy < 50; ++iy)
	{
		double y = iy * 0.2 + 0.1;
		for (int ix = -50; ix < 50; ++ix)
		{
			double x = ix * 0.2 + 0.1;
			m[ix + 50][iy + 50] = potential(x, y);
		}
	}
	std::vector<double> xfrom(441), yfrom(441), xlen(441), ylen(441);
	std::vector<double> arrowcolor(441);
	for (int iy = -10; iy <= 10; ++iy)
	{
		for (int ix = -10; ix <= 10; ++ix)
		{
			size_t jx = (ix + 10);
			size_t jy = (iy + 10);
			double xlen_ = fieldx(ix, iy);
			double ylen_ = fieldy(ix, iy);
			double len = std::sqrt(xlen_ * xlen_ + ylen_ * ylen_);
			xlen_ = xlen_ / len * 0.8;
			ylen_ = ylen_ / len * 0.8;
			xlen[jy * 21 + jx] = xlen_;
			ylen[jy * 21 + jx] = ylen_;
			xfrom[jy * 21 + jx] = ix - xlen_ / 2.;
			yfrom[jy * 21 + jx] = iy - ylen_ / 2.;
			arrowcolor[jy * 21 + jx] = potential(ix - xlen_ / 2., iy - ylen_ / 2.);
		}
	}

	/* options for Colormap
	title           ... title.
	axis            ... set of axes to scale lines. (e.g. plot::axis = "x1y2")

	//contour options
	with_contour    ... plot contour
	without_surface ... plot without surface. Basically used with "with_contour". 
	cntrsmooth      ... interpolation by CntrSmooth::linear, cubicspline, bspline
	cntropints      ... the number of lines for cpline and bspline
	cntrorder       ... order for bspline, [2, 10]
	cntrlevels_auto
	cntrlevels_discrete ... cntrlevels_discrete <z1>, <z2>, ... generate contours at z1, z2, ...
	cntrlevels_incremental ... cntrlevels_incremental <start>, <incr>, <end> generate contours at start, start+incr, start+2*incr, ...
	cntrcolor
	variable_cntrcolor ... color contours with its z value.
	cntrlinetype    ... linetype. see gnuplot's "linetype" option.
	cntrlinewidth
	*/

	{
		GPMMultiPlot multi(output_filename, 1, 2, 1200, 600);

		GPMCanvasCM g1(output_filename + ".map_tmp");
		g1.ShowCommands(true);
		g1.EnableInMemoryDataTransfer(enable_in_memory_data_transfer); // Enable or disable datablock feature of gnuplot
		g1.SetTitle("example\\_colormap");
		g1.SetPaletteDefined({ {0, "yellow" }, { 4.5, "red" }, { 5., "black" }, { 5.5, "blue"}, { 10, "cyan" } });
		g1.SetSizeRatio(-1);
		g1.SetXLabel("x");
		g1.SetYLabel("y");
		g1.SetXRange(-10, 10);
		g1.SetYRange(-10, 10);
		g1.SetCBRange(-5, 5);
		g1.PlotColormap(m, xrange, yrange, plot::title = "notitle").
			PlotVectors(xfrom, yfrom, xlen, ylen, plot::title = "notitle", plot::color = "white");

		//sleep for a short time to avoid the output image broken by multiplot.
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		GPMCanvasCM g2(output_filename + ".cntr_tmp");
		g2.ShowCommands(true);
		g2.EnableInMemoryDataTransfer(enable_in_memory_data_transfer); // Enable or disable datablock feature of gnuplot
		g2.SetTitle("example\\_contour");
		g2.SetPaletteDefined({ {0, "yellow" }, { 4.5, "red" }, { 5., "black" }, { 5.5, "blue"}, { 10, "cyan" } });
		g2.SetSizeRatio(-1);
		g2.SetXLabel("x");
		g2.SetYLabel("y");
		g2.SetXRange(-10, 10);
		g2.SetYRange(-10, 10);
		g2.SetCBRange(-5, 5);
		g2.PlotColormap(m, xrange, yrange, plot::title = "notitle",
						plot::with_contour, plot::without_surface, plot::variable_cntrcolor,
						plot::cntrlevels_incremental = { -20., 0.2, 20. }).
			PlotVectors(xfrom, yfrom, xlen, ylen, plot::title = "notitle", plot::variable_color = arrowcolor);
	}

	if (!enable_in_memory_data_transfer)
	{
		GPMCanvasCM g1(output_filename + ".fileplot.png");
		g1.ShowCommands(true);
		g1.SetTitle("example\\_colormap");
		g1.SetPaletteDefined({ {0, "yellow" }, { 4.5, "red" }, { 5., "black" }, { 5.5, "blue"}, { 10, "cyan" } });
		g1.SetSizeRatio(-1);
		g1.SetXLabel("x");
		g1.SetYLabel("y");
		g1.SetXRange(-10, 10);
		g1.SetYRange(-10, 10);
		g1.SetCBRange(-5, 5);
		g1.PlotColormap(output_filename + ".map_tmp.tmp0.txt", "1", "2", "5", plot::title = "notitle").
			PlotVectors(output_filename + ".map_tmp.tmp1.txt", "1", "2", "3", "4", plot::title = "notitle", plot::color = "white");
	}
	return 0;
}

#endif