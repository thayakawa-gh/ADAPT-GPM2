#ifndef GPM2_GPMCORE_H
#define GPM2_GPMCORE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <string_view>
#include <cfloat>
#include <ADAPT/CUF/Format.h>

namespace adapt
{

namespace gpm2
{

class GPMMultiPlotter;

enum class Style { none, lines, points, linespoints, dots, impulses, boxes, steps, fsteps, histeps, };
enum class Smooth { none, unique, frequency, cumulative, cnormal, kdensity, csplines, acsplines, bezier, sbezier, };
enum class ArrowHead { head = 0, heads = 1, noheads = 2, filled = 0 << 2, empty = 1 << 2, nofilled = 2 << 2, };

class GPMCanvas
{
public:

	friend class GPMMultiPlot;

	GPMCanvas(std::string_view output, double sizex = 0., double sizey = 0.);
	GPMCanvas();
	GPMCanvas(const GPMCanvas&) = delete;
	GPMCanvas(GPMCanvas&&) = delete;
	GPMCanvas& operator=(const GPMCanvas&) = delete;
	GPMCanvas& operator=(GPMCanvas&&) = delete;
	virtual ~GPMCanvas();

	void SetLabel(std::string_view axis, std::string_view label);
	void SetRange(std::string_view axis, double min, double max);
	void SetRangeMin(std::string_view axis, double min);
	void SetRangeMax(std::string_view axis, double max);
	void SetLog(std::string_view axis, double base = 10);
	void SetFormat(std::string_view axis, std::string_view fmt);
	void SetDataTime(std::string_view axis, std::string_view fmt = std::string());
	bool IsDateTimeEnabled(std::string_view axis);

	//見出しを数字から与えられた文字列に置き換える。
	template <class ...Args>
	void SetTics(std::string_view axis, Args&& ...args);
private:
	template <class ...Args>
	void SetTics_make(std::string& tics, std::string_view label, double value, Args&& ...args);
	template <class ...Args>
	void SetTics_make(std::string& tics, std::string_view label, double value, int level, Args&& ...args);
	void SetTics_make(std::string& tics);
public:

	void SetTics(double interval);
	void SetTics(double begin, double end, double interval);
	void SetMTics(double interval);
	void SetMTics(double begin, double end, double interval);

	void SetXticsRotate(double ang);

	void SetGrid(std::string_view color = "", int type = -2, int width = -1);

	void SetSize(double x, double y);
	void SetSizeRatio(double ratio);//-1を与えるとticsの幅が等しくなる。

	void SetPaletteDefined(const std::vector<std::pair<double, std::string>>& color);
	void SetPaletteRGBFormulae(int x, int y, int z);
	void SetPaletteCubehelix(double start, double cycles, double saturation);
	void SetPaletteMaxcolors(int num);
	//void SetKeyOff();
	//void SetKey(std::string_view posx, std::string_view posy);//"left", "right", "top", "bottom", "outside", "below"をxyそれぞれに。

	void SetTitle(std::string_view title);

	void SetParametric();

	void SetLeftMargin(double w);
	void SetRightMargin(double w);
	void SetTopMargin(double w);
	void SetBottomMargin(double w);

	void SetMargins(double l, double r, double b, double t);

	void SetOutput(std::string_view output, double sizex, double sizey);
	void Reset();
	const std::string& GetOutput() const;

	template <class ...Args>
	void Command(Args&& ...args);
	void ShowCommands(bool b);

	FILE* GetPipe() const { return mPipe; }

	// Enable or disable datablock feature of Gnuplot
	// If disabled, temporary files are created to pass data to Gnuplot.
	void EnableInMemoryDataTransfer(bool b);
	bool IsInMemoryDataTransferEnabled();

	static void SetGnuplotPath(std::string_view path);
	static std::string GetGnuplotPath();

protected:

	std::string mOutput;
	FILE* mPipe;
	bool mShowCommands;
	bool mInMemoryDataTransfer; // Use datablock feature of Gnuplot if true (default: false)

	//When any axes (x, y, x2, y2, z) are contained within this variable, 
	//the values of them are treated as DateTime even if the type of values are std::string or a type that is convertible to std::string.
	std::set<std::string> mDateTimeAxes;
	template <class = void>
	struct Paths
	{
		static std::string msGnuplotPath;
		static const std::string msDefaultGnuplotPath;
		static FILE* msGlobalPipe;//multiplotなどを利用する際のグローバルなパイプ。これがnullptrでない場合、mPipe==mGlobalPipeとなる。
	};
};


inline GPMCanvas::GPMCanvas(std::string_view output, double sizex, double sizey)
	: mOutput(output), mPipe(nullptr), mShowCommands(false), mInMemoryDataTransfer(false)
{
	if (Paths<>::msGlobalPipe != nullptr) mPipe = Paths<>::msGlobalPipe;
	else
	{
#if defined(_WIN32)
		if ((mPipe = _popen(GetGnuplotPath().c_str(), "w")) == nullptr)
#elif defined(__linux__) || defined(__APPLE__)
		if ((mPipe = popen(GetGnuplotPath().c_str(), "w")) == nullptr)
#endif
		{
			std::cerr << "Gnuplot cannot open. " << GetGnuplotPath() << std::endl;
		}
		else
		{
			SetOutput(output, sizex, sizey);
		}
	}
	if (mPipe)
	{
		Command("set bars small");
		Command("set palette defined ( 0 '#000090',1 '#000fff',2 '#0090ff',3 '#0fffee',4 '#90ff70',5 '#ffee00',6 '#ff7000',7 '#ee0000',8 '#7f0000')");
	}
}
inline GPMCanvas::GPMCanvas()
	: mOutput("ADAPT_GPM2_TMPFILE"), mPipe(nullptr), mShowCommands(false), mInMemoryDataTransfer(false)
{
	if (Paths<>::msGlobalPipe != nullptr) mPipe = Paths<>::msGlobalPipe;
	else
	{
#if defined(_WIN32)
		if ((mPipe = _popen(GetGnuplotPath().c_str(), "w")) == nullptr)
#elif defined(__linux__) || defined(__APPLE__)
		if ((mPipe = popen(GetGnuplotPath().c_str(), "w")) == nullptr)
#endif
		{
			std::cerr << "Gnuplot cannot open. " << GetGnuplotPath() << std::endl;
		}
	}
	if (mPipe)
	{
		Command("set bars small");
		Command("set palette defined ( 0 '#000090',1 '#000fff',2 '#0090ff',3 '#0fffee',4 '#90ff70',5 '#ffee00',6 '#ff7000',7 '#ee0000',8 '#7f0000')");
	}
}
inline GPMCanvas::~GPMCanvas()
{
	if (mPipe != nullptr && mPipe != Paths<>::msGlobalPipe)
	{
		Command("exit");
#if defined(_WIN32)
		_pclose(mPipe);
#elif defined(__linux__) || defined(__APPLE__)
		pclose(mPipe);
#endif
	}
	mPipe = nullptr;
}

inline void GPMCanvas::SetLabel(std::string_view axis, std::string_view label)
{
	Command(Format("set %slabel '%s'", axis, label));
}
inline void GPMCanvas::SetRange(std::string_view axis, double min, double max)
{
	Command(Format("set %srange [%lf:%lf]", axis, min, max));
}
inline void GPMCanvas::SetRangeMin(std::string_view axis, double min)
{
	Command(Format("set %srange [%lf:]", axis, min));
}
inline void GPMCanvas::SetRangeMax(std::string_view axis, double max)
{
	Command(Format("set %srange [:%lf]", axis, max));
}
inline void GPMCanvas::SetLog(std::string_view axis, double base)
{
	Command(Format("set logscale %s %lf", axis, base));
}
inline void GPMCanvas::SetFormat(std::string_view axis, std::string_view fmt)
{
	Command(Format("set format %s \"%s\"", axis, fmt));
}
inline void GPMCanvas::SetDataTime(std::string_view axis, std::string_view fmt)
{
	mDateTimeAxes.insert(std::string(axis));
	Command(Format("set %sdata time", axis));
	if (!fmt.empty()) Command(Format("set timefmt \"%s\"", fmt));
}
inline bool GPMCanvas::IsDateTimeEnabled(std::string_view axis)
{
	return mDateTimeAxes.find(std::string(axis)) != mDateTimeAxes.end();
}

inline void GPMCanvas::SetTics_make(std::string& tics)
{
	tics.erase(tics.end() - 2, tics.end());
	tics += ")";
	//途中。
}
inline void GPMCanvas::SetXticsRotate(double ang)
{
	Command(Format("set xtics rotate by %lf", ang));
}

inline void GPMCanvas::SetGrid(std::string_view color, int type, int width)
{
	std::string c;
	if (!color.empty()) c += Format(" linecolor rgb \"%s\"", color);
	if (type != -2) c += Format(" linetype %d", type);
	if (width != -1) c += Format(" linewidth %d", width);
	Command("set grid" + c);
}

inline void GPMCanvas::SetSize(double x, double y)
{
	Command(Format("set size %lf, %lf", x, y));
}
inline void GPMCanvas::SetSizeRatio(double ratio)
{
	Command(Format("set size ratio %lf", ratio));
}
inline void GPMCanvas::SetPaletteDefined(const std::vector<std::pair<double, std::string>>& color)
{
	std::string x;
	for (const auto& c : color)
	{
		x += (std::to_string(c.first) + " \"" + c.second + "\", ");
	}
	x.erase(x.end() - 2, x.end());
	Command("set palette defined (" + x + ")");
}
inline void GPMCanvas::SetPaletteRGBFormulae(int x, int y, int z)
{
	Command(Format("set palette rgbformulae %d, %d, %d", x, y, z));
}
inline void GPMCanvas::SetPaletteCubehelix(double start, double cycles, double saturation)
{
	Command(Format("set palette cubehelix start %lf, cycles %lf, saturation %lf", start, cycles, saturation));
}
inline void GPMCanvas::SetPaletteMaxcolors(int num)
{
	Command(Format("set palette naxcolors %d", num));
}
inline void GPMCanvas::SetTitle(std::string_view title)
{
	Command(Format("set title '%s'", title));
}
inline void GPMCanvas::SetParametric()
{
	Command("set parametric");
}

inline void GPMCanvas::SetLeftMargin(double w)
{
	Command(Format("set lmargin ", w));
}
inline void GPMCanvas::SetRightMargin(double w)
{
	Command(Format("set rmargin ", w));
}
inline void GPMCanvas::SetBottomMargin(double w)
{
	Command(Format("set bmargin ", w));
}
inline void GPMCanvas::SetTopMargin(double w)
{
	Command(Format("set tmargin ", w));
}

inline void GPMCanvas::SetMargins(double l, double r, double b, double t)
{
	SetLeftMargin(l);
	SetRightMargin(r);
	SetBottomMargin(b);
	SetTopMargin(t);
}

inline void GPMCanvas::SetOutput(std::string_view output, double sizex, double sizey)
{
	if (output.size() > 4)
	{
		std::string_view extension(output.substr(output.size() - 4, 4));
		std::string repout = ReplaceStr(output, "\\", "/");
		if (extension == ".png")
		{
			if (sizex == 0 && sizey == 0) sizex = 800, sizey = 600;
			Command(Format("set terminal pngcairo enhanced size %d, %d\nset output '" + repout + "'", sizex, sizey));
		}
		else if (extension == ".eps")
		{
			if (sizex == 0 && sizey == 0) sizex = 6, sizey = 4.5;
			Command(Format("set terminal epscairo enhanced size %din, %din\nset output '" + repout + "'", sizex, sizey));
		}
		else if (extension == ".pdf")
		{
			if (sizex == 0 && sizey == 0) sizex = 6, sizey = 4.5;
			Command(Format("set terminal pdfcairo enhanced size %lfin, %lfin\nset output '" + repout + "'", sizex, sizey));
		}
	}
	else if (output == "wxt");
	else std::cout << "WARNING : " << output << " is not a terminal or has no valid extension. Default terminal is selected." << std::endl;

}
inline void GPMCanvas::Reset()
{
	Command("reset");
}
inline const std::string& GPMCanvas::GetOutput() const
{
	return mOutput;
}

template <class ...Args>
inline void GPMCanvas::Command(Args&& ...args)
{
	adapt::Print(mPipe, std::forward<Args>(args)...);
	if (mShowCommands) adapt::Print(std::cout, std::forward<Args>(args)...);
}
inline void GPMCanvas::ShowCommands(bool b)
{
	mShowCommands = b;
}

inline void GPMCanvas::EnableInMemoryDataTransfer(bool b)
{
	mInMemoryDataTransfer = b;
}

inline bool GPMCanvas::IsInMemoryDataTransferEnabled()
{
	return mInMemoryDataTransfer;
}

inline void GPMCanvas::SetGnuplotPath(std::string_view path)
{
	Paths<>::msGnuplotPath = path;
}
inline std::string GPMCanvas::GetGnuplotPath()
{
	if (!Paths<>::msGnuplotPath.empty()) return Paths<>::msGnuplotPath;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif 
	if (const char* p = std::getenv("GNUPLOT_PATH")) return std::string(p);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	return Paths<>::msDefaultGnuplotPath;
}
template <class T>
std::string GPMCanvas::Paths<T>::msGnuplotPath = "";
#ifdef _WIN32
template <class T>
const std::string GPMCanvas::Paths<T>::msDefaultGnuplotPath = "C:/Progra~1/gnuplot/bin/gnuplot.exe";
#else
template <class T>
const std::string GPMCanvas::Paths<T>::msDefaultGnuplotPath = "gnuplot";
#endif
template <class T>
FILE* GPMCanvas::Paths<T>::msGlobalPipe = nullptr;

template <class ...Args>
inline void GPMCanvas::SetTics(std::string_view axis, Args&& ...args)
{
	std::string c = "(";
	SetTics_make(c, std::forward<Args>(args)...);
	Command(Format("set %s tics %s", axis, c));
}
template <class ...Args>
inline void GPMCanvas::SetTics_make(std::string& tics, std::string_view label, double value, Args&& ...args)
{
	tics += Format("'%s' %lf, ", label, value);
	SetTics_make(tics, std::forward<Args>(args)...);
}
template <class ...Args>
inline void GPMCanvas::SetTics_make(std::string& tics, std::string_view label, double value, int level, Args&& ...args)
{
	tics += Format("'%s' %lf %d, ", label, value, level);
	SetTics_make(tics, std::forward<Args>(args)...);
}

class GPMMultiPlot
{
public:
	GPMMultiPlot(std::string_view outputname, int row, int column, double sizex = 0., double sizey = 0.);
	~GPMMultiPlot();

	void Begin(std::string_view output, int row, int column, double sizex = 0., double sizey = 0.);
	void End();

	template <class ...Args>
	void Command(Args&& ...args);
private:
};

inline GPMMultiPlot::GPMMultiPlot(std::string_view output, int row, int column, double sizex, double sizey)
{
	Begin(output, row, column, sizex, sizey);
}
inline GPMMultiPlot::~GPMMultiPlot()
{
	End();
}
inline void GPMMultiPlot::Begin(std::string_view output, int row, int column, double sizex, double sizey)
{
	if (GPMCanvas::Paths<>::msGlobalPipe != nullptr)
	{
		std::cerr << "Gnuplot has already been open. " << GPMCanvas::GetGnuplotPath() << std::endl;
		return;
	}
	if ((GPMCanvas::Paths<>::msGlobalPipe = _popen(GPMCanvas::GetGnuplotPath().c_str(), "w")) == nullptr)
	{
		std::cerr << "Gnuplot cannot open. " << GPMCanvas::GetGnuplotPath() << std::endl;
	}
	else
	{
		Command("set bars small");
		Command("set palette defined ( 0 '#000090',1 '#000fff',2 '#0090ff',3 '#0fffee',4 '#90ff70',5 '#ffee00',6 '#ff7000',7 '#ee0000',8 '#7f0000')");

		if (output.size() > 4)
		{
			std::string_view extension = output.substr(output.size() - 4, 4);
			std::string repout = ReplaceStr(output, "\\", "/");
			if (extension == ".png")
			{
				if (sizex == 0 && sizey == 0) sizex = 800 * column, sizey = 600 * row;
				Command(Format("set terminal pngcairo enhanced size %d, %d\nset output '%s'", (int)sizex, (int)sizey, repout));
			}
			else if (extension == ".eps")
			{
				if (sizex == 0 && sizey == 0) sizex = 6 * column, sizey = 4.5 * row;
				Command(Format("set terminal epscairo enhanced size %lfin, %lfin\nset output '%s'", sizex, sizey, repout));
			}
			else if (extension == ".pdf")
			{
				if (sizex == 0 && sizey == 0) sizex = 6 * column, sizey = 4.5 * row;
				Command(Format("set terminal pdfcairo enhanced size %lfin, %lfin\nset output '%s'", sizex, sizey, repout));
			}
		}
		else if (output == "wxt");
		else std::cout << "WARNING : " << output << " is not a terminal or has no valid extension. Default terminal is selected." << std::endl;

		Command("set multiplot layout " + std::to_string(row) + ", " + std::to_string(column));
	}
}
inline void GPMMultiPlot::End()
{
	if (GPMCanvas::Paths<>::msGlobalPipe != nullptr)
	{
		Command("unset multiplot");
		_pclose(GPMCanvas::Paths<>::msGlobalPipe);
		GPMCanvas::Paths<>::msGlobalPipe = nullptr;
	}
}
template <class ...Args>
inline void GPMMultiPlot::Command(Args&& ...args)
{
	adapt::Print(GPMCanvas::Paths<>::msGlobalPipe, std::forward<Args>(args)...);
}

}

}

#endif