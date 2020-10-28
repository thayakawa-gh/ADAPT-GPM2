# ADAPT-GPM2
GPM2 is a C++ header-only library to manage Gnuplot. This is designed to draw lines, points, vectors, colormap etc. from std::vector or Matrix data points without Gnuplot's weird command "using", "with ~". Basically it does not require the knowledge of Gnuplot.

In this library, plot functions can be given various options such as title, style, errorbar, variable color, etc. in the form of unordered keyword arguments.
```cpp
GPMCanvas::SetGnuplotPath("path to gnuplot");

std::string norm = std::to_string(250. / std::sqrt(2 * 3.1415926535));
std::string equation = norm + "*exp(-x*x/2)";

std::mt19937_64 mt(0);
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

GPMCanvas2D g("example_2d.png");
g.SetTitle("example\\_2d");
g.SetXRange(-4.0, 4.0);
g.SetXLabel("x");
g.SetYLabel("y");
g.PlotPoints(equation, plot::title = "mu = 0, sigma = 1",
             plot::style = Style::lines).
  PlotPoints(x1, y1, plot::xerrorbar = 0.125, plot::yerrorbar = e1,
	 plot::title = "data", plot::color = "black",
	 plot::style = Style::points, plot::pointtype = 7, plot::pointsize = 0.5);
```
<img src="https://user-images.githubusercontent.com/53743073/71127820-23f09c80-222f-11ea-945e-a5b598390388.png" width="480px">
<img src="https://user-images.githubusercontent.com/53743073/71127869-3e2a7a80-222f-11ea-839c-06acf20545f1.png" width="960px">
<img src="https://user-images.githubusercontent.com/53743073/71127885-484c7900-222f-11ea-99b5-a6b093de109f.png" width="480px">

GPM2 is a part of ADAPT, which is a library for statistical aggregation, analysis, processing and 2D/3D visualization with hierarchical data container, however, almost all ADAPT is available only for the members of the laboratory I belong to.
