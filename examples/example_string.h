#ifndef EXAMPLE_STRING_H
#define EXAMPLE_STRING_H

#include <ADAPT/GPM2/GPMCanvas.h>

using namespace adapt::gpm2;

int example_string()
{
    std::vector<std::string> x;
    std::vector<double> y;

    x.push_back("label-one");   y.push_back(1);
    x.push_back("label-two");   y.push_back(2);
    x.push_back("label-three"); y.push_back(3);
    x.push_back("label-four");  y.push_back(4);
    x.push_back("label-five");  y.push_back(5);
    x.push_back("label-six");   y.push_back(6);
    x.push_back("label-seven");   y.push_back(7);
    x.push_back("label-eight"); y.push_back(8);
    x.push_back("label-nine");  y.push_back(9);
    x.push_back("label-ten");  y.push_back(10);

    GPMCanvas2D g("string_label.png");
    g.ShowCommands(true);
    g.SetXRange(-1, 10);
    g.SetYRange(0, 11);
    g.SetXticsRotate(-45);
    g.PlotPoints(x, y, plot::style = Style::boxes, plot::title = "notitle");

    return 0;
}

#endif