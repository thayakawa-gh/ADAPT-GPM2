#include <ADAPT/GPM2/GPMCanvas.h>
#include <string>

using namespace adapt::gpm2;

int example_for_loop()
{
	GPMCanvas2D g("example_for_loop.png");

	auto buf = g.GetBuffer();
	for (int i = 0; i < 5; ++i)
	{
		std::string eq = std::to_string(i) + "*x";
		buf = buf.PlotPoints(eq, plot::title = eq, plot::style = Style::lines);
	}
	buf.Flush();
}