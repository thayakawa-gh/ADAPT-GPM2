# ADAPT-GPM2
GPM2 is a C++ header-only library to manage Gnuplot. This is designed to draw lines, points, vectors, colormap etc. from std::vector or Matrix data points without Gnuplot's weird command "using", "with ~". Basically it does not require the knowledge of Gnuplot.

In this library, plot functions can be given various options such as title, style, errorbar, variable color, etc. in the form of unordered keyword arguments.
```
GPMCanvas::SetGnuplotPath("path to gnuplot");

std::vector<double> x;
std::vector<double> y;
std::vector<double> e;
std::vector<double> c;

GPMCanvas2D g("output.png");
g.SetXRange(-10, 10);
g.SetCBRange(-10, 10);
g.SetXLabel("x");
g.SetYLabel("y");

g.PlotPoints(x, y, plot::variable_color = c,
            plot::style = plot::LINES, plot::title = "variable color lines").
  PlotPoints(x, y, plot::xerrorbar = 1.0, plot::yerrorbar = e,
            plot::style = plot::POINTS, plot::title = "points with error bars");
```
<img src="https://user-images.githubusercontent.com/53743073/62621795-c599e000-b957-11e9-9d69-f20dfaefec81.png" width="480px">
<img src="https://user-images.githubusercontent.com/53743073/62621810-d185a200-b957-11e9-8489-740bdbac29d8.png" width="480px">

GPM2 is a part of ADAPT, which is a library for statistical aggregation, analysis, processing and 2D/3D visualization with hierarchical data container, however, almost all ADAPT is available only for the members of the laboratory I belong to.
