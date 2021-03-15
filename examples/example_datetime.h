#ifndef GPM2_EXAMPLE_DATETIME_H
#define GPM2_EXAMPLE_DATETIME_H

#include <ADAPT/GPM2/GPMCanvas.h>
#if defined ADAPT_USE_CPP20
#include <ranges>
#endif
#include <random>
#include <ctime>
#include <chrono>

using namespace adapt::gpm2;

std::random_device rd;
std::mt19937_64 mt(rd());

int GetTested(int d)
{
	double a = d * d * 0.05;
	double b = a * std::pow((std::sin(2 * 3.141592 * d / 120) + 1) / 2, 3) + d;
	return b <= 0 ? 0 : std::poisson_distribution<>(b)(mt);
}
int GetPositive(int t)
{
	return t == 0 ? 0 : std::binomial_distribution<>(t, 0.05)(mt);
}

std::pair<int, int> GetDate(int a)
{
	static constexpr std::array<int, 12> x = { 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
	int m = 0;
	int d = 0;
	int p = 0;
	for (int i = 0; i < 12; ++i)
	{
		if (a + 1 <= x[i]) { m = i + 1, d = a + 1 - p; break; };
		p = x[i];
	}
	return { m, d };
}

struct Accumulator
{
	int operator()(int t) const { return sum += t; }
	mutable int sum;
};
int example_datetime(const std::string output_filename = "example_datetime.png", const bool enable_in_memory_data_transfer = false)
{
#if defined ADAPT_USE_CPP20
	{
		std::vector<int> yr(366);
		std::iota(yr.begin(), yr.end(), 0);
		auto x = std::views::all(yr) | std::views::transform([](int a) { auto [m, d] = GetDate(a); return adapt::Format("2020-%d-%d", m, d); });
		auto y = std::views::all(yr) | std::views::transform([](int a) { return GetTested(a); });
		auto y2 = y | std::views::transform([](int t) { return GetPositive(t); });
		auto y3 = y | std::views::transform(Accumulator{});
		auto y4 = y2 | std::views::transform(Accumulator{});
		GPMCanvas2D g(output_filename);
		g.ShowCommands(true);
		g.EnableInMemoryDataTransfer(enable_in_memory_data_transfer);
		g.SetXticsRotate(-45);
		g.SetTitle("example\\_datetime");
		g.SetYLabel("per day");
		g.SetY2Label("total");
		g.SetXDataTime("%Y-%m-%d");
		g.SetFormatX("%02m/%02d");
		g.PlotPoints(x, y, plot::title = "tested", plot::style = Style::steps, plot::fillsolid = 0.5).
			PlotPoints(x, y2, plot::title = "positive", plot::style = Style::steps, plot::fillsolid = 0.5).
			PlotLines(x, y3, plot::title = "tested\\_total", plot::axis = "x1y2", plot::linewidth = 3).
			PlotLines(x, y4, plot::title = "positive\\_total", plot::axis = "x1y2", plot::linewidth = 3);
	}
#else
	{
		std::vector<std::string> x;
		std::vector<int> y, y2, y3, y4;
		int tsum = 0, psum = 0;
		for (int a = 0; a < 366; ++a)
		{
			auto [m, d] = GetDate(a);
			x.push_back(adapt::Format("2020-%d-%d", m, d));
			auto t = GetTested(a);
			auto p = GetPositive(t);
			tsum += t;
			psum += p;
			y.push_back(t);
			y2.push_back(p);
			y3.push_back(tsum);
			y4.push_back(psum);
		}
		GPMCanvas2D g(output_filename);
		g.ShowCommands(true);
		g.EnableInMemoryDataTransfer(enable_in_memory_data_transfer);
		g.SetXticsRotate(-45);
		g.SetTitle("example\\_datetime");
		g.SetYLabel("per day");
		g.SetY2Label("total");
		g.SetXDataTime("%Y-%m-%d");
		g.SetFormatX("%02m/%02d");
		g.PlotPoints(x, y, plot::title = "tested", plot::style = Style::steps, plot::fillsolid = 0.5).
			PlotPoints(x, y2, plot::title = "positive", plot::style = Style::steps, plot::fillsolid = 0.5).
			PlotLines(x, y3, plot::title = "tested\\_total", plot::axis = "x1y2", plot::linewidth = 3).
			PlotLines(x, y4, plot::title = "positive\\_total", plot::axis = "x1y2", plot::linewidth = 3);
	}
#endif
	return 0;
}

#endif