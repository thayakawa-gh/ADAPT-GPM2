#include "example_2d.h"
#include "example_colormap.h"
#include "example_filledcurve.h"
#include "example_string.h"

int main()
{
	//Default GNUPLOT path is "C:/Progra~1/gnuplot/bin/gnuplot.exe" for windows, and not set for the other envitonments.
	//There are 2 ways to overwrite it.
	//1. Call function "GPMCanvas::SetGnuplotPath("path to gnuplot")".
	//2. Define environment variable "GNUPLOT_PATH=path to gnuplot".

	example_2d();

	example_colormap();

	example_filledcurve();

	example_string();
}