#ifndef GPM2_GPMCANVAS_H
#define GPM2_GPMCANVAS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cfloat>
#include <ADAPT/CUF/Matrix.h>
#include <ADAPT/CUF/KeywordArgs.h>
#include <ADAPT/CUF/Format.h>
#include <ADAPT/CUF/Function.h>
#include <ADAPT/GPM2/GPMArrayData.h>

namespace adapt
{

namespace gpm2
{

class GPMMultiPlot;

enum class Style { none, lines, points, linespoints, dots, impulses, boxes, steps, fsteps, histeps, };
enum class Smooth { none, unique, frequency, cumulative, cnormal, kdensity, csplines, acsplines, bezier, sbezier, };
enum class ArrowHead { head = 0, heads = 1, noheads = 2, filled = 0 << 2, empty = 1 << 2, nofilled = 2 << 2, };

class GPMCanvas
{
public:

	friend class GPMMultiPlot;

	GPMCanvas(const std::string& output, double sizex = 0., double sizey = 0.);
	GPMCanvas();
	GPMCanvas(const GPMCanvas&) = delete;
	GPMCanvas(GPMCanvas&&) = delete;
	GPMCanvas& operator=(const GPMCanvas&) = delete;
	GPMCanvas& operator=(GPMCanvas&&) = delete;
	virtual ~GPMCanvas();

	void SetLabel(const std::string& axis, const std::string& label);
	void SetRange(const std::string& axis, double min, double max);
	void SetRangeMin(const std::string& axis, double min);
	void SetRangeMax(const std::string& axis, double max);
	void SetLog(const std::string& axis, double base = 10);

	//見出しを数字から与えられた文字列に置き換える。
	template <class ...Args>
	void SetTics(const std::string& axis, Args&& ...args);
private:
	template <class ...Args>
	void SetTics_make(std::string& tics, const std::string& label, double value, Args&& ...args);
	template <class ...Args>
	void SetTics_make(std::string& tics, const std::string& label, double value, int level, Args&& ...args);
	void SetTics_make(std::string& tics);
public:

	void SetTics(double interval);
	void SetTics(double begin, double end, double interval);
	void SetMTics(double interval);
	void SetMTics(double begin, double end, double interval);

	void SetXticsRotate(double ang);

	void SetGrid(const std::string& color = "", int type = -2, int width = -1);

	void SetSize(double x, double y);
	void SetSizeRatio(double ratio);//-1を与えるとticsの幅が等しくなる。

	void SetPaletteDefined(const std::vector<std::pair<double, std::string>>& color);
	void SetPaletteRGBFormulae(int x, int y, int z);
	void SetPaletteCubehelix(double start, double cycles, double saturation);
	void SetPaletteMaxcolors(int num);
	//void SetKeyOff();
	//void SetKey(const std::string& posx, const std::string& posy);//"left", "right", "top", "bottom", "outside", "below"をxyそれぞれに。

	void SetTitle(const std::string& title);

	void SetParametric();

	void SetLeftMargin(double w);
	void SetRightMargin(double w);
	void SetTopMargin(double w);
	void SetBottomMargin(double w);

	void SetMargins(double l, double r, double b, double t);

	void SetOutput(const std::string& output, double sizex, double sizey);
	void Reset();
	const std::string& GetOutput() const;

	template <class ...Args>
	void Command(Args&& ...args);
	void ShowCommands(bool b);

	// Enable or disable datablock feature of Gnuplot
	// If disabled, temporary files are created to pass data to Gnuplot.
	void EnableInMemoryDataTransfer(bool b);
	bool IsInMemoryDataTransferEnabled();

	static void SetGnuplotPath(const std::string& path);
	static std::string GetGnuplotPath();

protected:

	std::string mOutput;
	FILE* mPipe;
	bool mShowCommands;
	bool mInMemoryDataTransfer; // Use datablock feature of Gnuplot if true (default: false)
	template <class = void>
	struct Paths
	{
		static std::string msGnuplotPath;
		static const std::string msDefaultGnuplotPath;
		static FILE* msGlobalPipe;//multiplotなどを利用する際のグローバルなパイプ。これがnullptrでない場合、mPipe==mGlobalPipeとなる。
	};
};


inline GPMCanvas::GPMCanvas(const std::string& output, double sizex, double sizey)
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

inline void GPMCanvas::SetLabel(const std::string& axis, const std::string& label)
{
	Command(Format("set %slabel '%s'", axis, label));
}
inline void GPMCanvas::SetRange(const std::string& axis, double min, double max)
{
	Command(Format("set %srange [%lf:%lf]", axis, min, max));
}
inline void GPMCanvas::SetRangeMin(const std::string& axis, double min)
{
	Command(Format("set %srange [%lf:]", axis, min));
}
inline void GPMCanvas::SetRangeMax(const std::string& axis, double max)
{
	Command(Format("set %srange [:%lf]", axis, max));
}

inline void GPMCanvas::SetLog(const std::string& axis, double base)
{
	Command(Format("set logscale %s %lf", axis, base));
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

inline void GPMCanvas::SetGrid(const std::string& color, int type, int width)
{
	std::string c;
	if (!color.empty()) c += " linecolor rgb \"" + color + "\"";
	if (type != -2) c += " linetype " + std::to_string(type);
	if (width != -1) c += " linewidth " + std::to_string(width);
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
inline void GPMCanvas::SetTitle(const std::string& title)
{
	Command("set title '" + title + "'");
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

inline void GPMCanvas::SetOutput(const std::string& output, double sizex, double sizey)
{
	if (output.size() > 4)
	{
		std::string extension = output.substr(output.size() - 4, 4);
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

inline void GPMCanvas::SetGnuplotPath(const std::string& path)
{
	Paths<>::msGnuplotPath = path;
}
inline std::string GPMCanvas::GetGnuplotPath()
{
	if (!Paths<>::msGnuplotPath.empty()) return Paths<>::msGnuplotPath;
	if (std::string p = GetEnv("GNUPLOT_PATH"); !p.empty()) return std::string(p);
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
inline void GPMCanvas::SetTics(const std::string& axis, Args&& ...args)
{
	std::string c = "(";
	SetTics_make(c, std::forward<Args>(args)...);
	Command("set " + axis + "tics " + c);
}
template <class ...Args>
inline void GPMCanvas::SetTics_make(std::string& tics, const std::string& label, double value, Args&& ...args)
{
	tics += ("'" + label + "' " + std::to_string(value) + ", ");
	SetTics_make(tics, std::forward<Args>(args)...);
}
template <class ...Args>
inline void GPMCanvas::SetTics_make(std::string& tics, const std::string& label, double value, int level, Args&& ...args)
{
	tics += ("'" + label + "' " + std::to_string(value) + " " + std::to_string(level) + ", ");
	SetTics_make(tics, std::forward<Args>(args)...);
}


namespace detail
{

using DataIterator = Variant<std::vector<double>::const_iterator, std::vector<std::string>::const_iterator>;

template <class OutputFunc>
inline void MakeDataObjectCommon(OutputFunc output_func, std::vector<DataIterator>& its, size_t size)
{
	auto f = Overload([](std::vector<double>::const_iterator& it, auto output_func) { output_func(" ", *it, print::end<'\0'>()); ++it; },
		[](std::vector<std::string>::const_iterator& it, auto output_func) { output_func(" ", it->c_str(), print::end<'\0'>()); ++it; });

	for (size_t i = 0; i < size; ++i)
	{
		for (auto& it : its)
		{
			it.Visit(f, output_func);
		}
		output_func("\n", print::end<>());
	}
}

template <class OutputFunc, class GetX, class GetY>
inline void MakeDataObjectCommon(OutputFunc output_func, const Matrix<double>& map, GetX getx, GetY gety)
{
	uint32_t xsize = map.GetSize(0);
	uint32_t ysize = map.GetSize(1);
	//xsize、ysizeはxcoord.size()-1、ycoord.size()-1にそれぞれ等しいはず。
	for (uint32_t iy = 0; iy < ysize; ++iy)
	{
		double y = gety(iy);
		double cy = gety.center(iy);
		for (uint32_t ix = 0; ix < xsize; ++ix)
		{
			double x = getx(ix);
			double cx = getx.center(ix);
			//output_func(std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(cx)
			//			+ " " + std::to_string(cy) + " " + std::to_string(map[ix][iy]));
			output_func(x, y, cx, cy, map[ix][iy]);
		}
		double x = getx(xsize);
		double cx = getx.center(xsize);
		//output_func(std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(cx)
		//			+ " " + std::to_string(cy) + " 0\n");
		output_func(x, y, cx, cy, " 0\n");
	}
	double y = gety(ysize);
	double cy = gety.center(ysize);
	for (uint32_t ix = 0; ix < xsize; ++ix)
	{
		double x = getx(ix);
		double cx = getx.center(ix);
		//output_func(std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(cx)
		//			+ " " + std::to_string(cy) + " 0");
		output_func(x, y, cx, cy, " 0");
	}
	//output_func(std::to_string(getx(xsize)) + " " + std::to_string(y) + " " + std::to_string(getx.center(xsize))
	//			+ " " + std::to_string(cy) + " 0");
	output_func(getx(xsize), y, getx.center(xsize), cy, " 0");
}
struct OutputFunc1
{
	template <class ...Args>
	void operator()(Args&& ...args) const
	{
		g->Command(std::forward<Args>(args)...);
	}
	GPMCanvas* g;
};
struct OutputFunc2
{
	template <class ...Args>
	void operator()(Args&& ...args)
	{
		Print(ofs, std::forward<Args>(args)...);
	}
	std::ofstream& ofs;
};
template <class ...Args>
inline void MakeDataObject(GPMCanvas* g, const std::string& name, Args&& ...args)
{
	if (g->IsInMemoryDataTransferEnabled()) 
	{
		// make datablock
		g->Command(name + " << EOD");
		OutputFunc1 output_func{ g };
		MakeDataObjectCommon(output_func, std::forward<Args>(args)...);
		g->Command("EOD");
	}
	else
	{
		// make file
		std::ofstream ofs(name);
		if (!ofs) throw InvalidArg("file \"" + name + "\" cannot open.");
		OutputFunc2 output_func{ ofs };
		MakeDataObjectCommon(output_func, std::forward<Args>(args)...);
	}
}

// Replace non-alphanumeric characters with '_'
inline std::string SanitizeForDataBlock(const std::string& str)
{
	std::string res = str;
	auto pos = res.begin();
	while (pos = std::find_if(pos, res.end(), [](char c) { return !isalnum(c); }), pos != res.end())
	{
		res.replace(pos, pos + 1, 1, '_');
		pos++;
	}
	return res;
}

template <class PointParam>
std::string PointPlotCommand(const PointParam& p)
{
	std::string c;
	if (p.mXErrorbar || p.mYErrorbar)
	{
		if (p.mStyle == Style::lines)
		{
			if (p.mXErrorbar && p.mYErrorbar)
				c += " xyerrorlines";
			else if (p.mXErrorbar)
				c += " xerrorlines";
			else
				c += " yerrorlines";
			if (p.mLineType != -2) c += " linetype " + std::to_string(p.mLineType);
			if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
			if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
			else if (p.mVariableColor) c += " linecolor palette";
		}
		else if (p.mStyle == Style::boxes)
		{
			//c += " boxes";
			if (p.mXErrorbar && p.mYErrorbar)
				c += " boxxyerrorbars";
			else if (p.mXErrorbar)
				std::cerr << "WARNING : Box style is incompatible with xerrorbar option. It is to be ignored." << std::endl;
			else
				c += " boxerrorbars";
			if (p.mLineType != -2) c += " linetype " + std::to_string(p.mLineType);
			if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
			if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
			else if (p.mVariableColor) c += " linecolor palette";
		}
		else
		{
			if (p.mStyle != Style::points)
				std::cerr << "WARNING : Only \"lines\", \"boxes\" or \"points\" styles are allowed with errobars. Style option is to be ignored." << std::endl;

			//c += " points";
			if (p.mXErrorbar && p.mYErrorbar)
				c += " xyerrorbars";
			else if (p.mXErrorbar)
				c += " xerrorbars";
			else if (p.mYErrorbar)
				c += " yerrorbars";
			if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
			if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
			if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";//pointのときも何故かlinecolorらしい。
			else if (p.mVariableColor) c += " palette";//しかしpalette指定の場合はlinecolorがいらない。謎。
		}
	}
	//ベクトル、エラーバー指定がない場合。
	else if (p.mStyle == Style::lines ||
			 p.mStyle == Style::impulses ||
			 p.mStyle == Style::steps ||
			 p.mStyle == Style::fsteps ||
			 p.mStyle == Style::histeps ||
			 p.mStyle == Style::boxes)
	{
		switch (p.mStyle)
		{
		case Style::lines: c += " lines"; break;
		case Style::impulses: c += " impulses"; break;
		case Style::steps: c += " steps"; break;
		case Style::fsteps: c += " fsteps"; break;
		case Style::histeps: c += " histeps"; break;
		case Style::boxes: c += " boxes"; break;
		default: break;
		}
		if (p.mLineType != -2) c += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColor) c += " linecolor palette";
	}
	else if (p.mStyle == Style::points)
	{
		c += " points";
		if (p.mLineType != -2)
			std::cerr << "WARNING : \"points\" style is incompatible with linetype option. It is to be ignored." << std::endl;
		if (p.mLineWidth != -1)
			std::cerr << "WARNING : \"points\" style is incompatible with linewidth option. It is to be ignored." << std::endl;
		if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
		if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";//pointのときも何故かlinecolorらしい。
		else if (p.mVariableColor) c += " palette";//しかしpalette指定の場合はlinecolorがいらない。謎。
	}
	else if (p.mStyle == Style::linespoints)
	{
		c += " linespoints";
		if (p.mLineType != -2) c += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
		if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
		if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColor) c += " linecolor palette";
	}
	else if (p.mStyle == Style::dots)
	{
		c += " dots";
		if (p.mLineType != -2)
			std::cerr << "WARNING : \"points\" style is incompatible with linetype option. It is to be ignored." << std::endl;
		if (p.mLineWidth != -1)
			std::cerr << "WARNING : \"points\" style is incompatible with linewidth option. It is to be ignored." << std::endl;
		if (p.mPointType != -1)
			std::cerr << "WARNING : \"dots\" style is incompatible with pointtype option. It is to be ignored." << std::endl;
		if (p.mPointSize != -1)
			std::cerr << "WARNING : \"dots\" style is incompatible with pointsize option. Use points with pointtype 7." << std::endl;
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColor) c += " palette ";
	}
	if (p.mSmooth != Smooth::none)
	{
		c += " smooth";
		switch (p.mSmooth)
		{
		case Smooth::unique: c += " unique"; break;
		case Smooth::frequency: c += " frequency"; break;
		case Smooth::cumulative: c += " cumulative"; break;
		case Smooth::cnormal: c += " cnormal"; break;
		case Smooth::kdensity: c += " kdensity"; break;
		case Smooth::csplines: c += " csplines"; break;
		case Smooth::acsplines: c += " acsplines"; break;
		case Smooth::bezier: c += " bezier"; break;
		case Smooth::sbezier: c += " sbezier"; break;
		default: break;
		}
	}
	return c;
}
template<class VectorParam>
std::string VectorPlotCommand(const VectorParam& v)
{
	std::string c;
	c += " vector ";
	if (v.mArrowHead != -1)
	{
		if ((v.mArrowHead & 0b11) == (int)ArrowHead::head) c += " head";
		else if ((v.mArrowHead & 0b11) == (int)ArrowHead::heads) c += " heads";
		else if ((v.mArrowHead & 0b11) == (int)ArrowHead::noheads) c += " noheads";
		if ((v.mArrowHead & 0b1100) == (int)ArrowHead::filled) c += " filled";
		else if ((v.mArrowHead & 0b1100) == (int)ArrowHead::empty) c += " empty";
		else if ((v.mArrowHead & 0b1100) == (int)ArrowHead::nofilled) c += " nofilled";
	}
	if (v.mLineType != -2) c += " linetype " + std::to_string(v.mLineType);
	if (v.mLineWidth != -1) c += " linewidth " + std::to_string(v.mLineWidth);
	if (!v.mColor.empty()) c += " linecolor '" + v.mColor + "'";
	else if (v.mVariableColor) c += " linecolor palette";
	return c;
}
template <class FilledCurveParam>
std::string FilledCurveplotCommand(const FilledCurveParam& f)
{
	std::string c;
	c += " filledcurves";

	if (!f.mClosed && !f.mAbove && !f.mBelow &&
		f.mBaseline.empty())
	{
		//何も指定のないデフォルトの場合、x1軸との間の領域を塗りつぶす。
		c += " x1";
	}

	if (f.mClosed) c += " closed";
	else if (f.mAbove) c += " above";
	else if (f.mBelow) c += " below";
	if (!f.mBaseline.empty()) c += " " + f.mBaseline;

	if (!f.mFillColor.empty()) c += " fillcolor '" + f.mFillColor + "'";
	else if (f.mVariableColor) c += " fillcolor palette";
	{
		std::string fs;
		if (f.mTransparent) fs += " transparent";
		if (f.mSolid != -1) fs += Format(" solid %lf", f.mSolid);
		else if (f.mPattern != -1) fs += Format(" pattern %d", f.mPattern);
		if (!fs.empty()) c += " fillstyle" + fs;
	}
	{
		std::string bd;
		if (f.mBorderType == -2) bd += Format(" noborder");
		else if (f.mBorderType != -3) bd += Format(" %d", f.mBorderType);
		if (!f.mBorderColor.empty()) bd += " linecolor '" + f.mBorderColor + "'";
		if (!bd.empty()) c += " border" + bd;
	}
	return c;
}
template <class GraphParam>
std::string ColormapPlotCommand(const GraphParam& p)
{
	return std::string();
}

#define DEF_GPMAXIS(AXIS, axis)\
template <class GPM>\
class GPM##AXIS##Axis : public GPM\
{\
public:\
	using GPM::GPM;\
	void Set##AXIS##Label(const std::string& label);\
	void Set##AXIS##Range(double min, double max);\
	void Set##AXIS##RangeMin(double min);\
	void Set##AXIS##RangeMax(double max);\
	void SetLog##AXIS(double base = 0);\
	template <class ...Args>\
	void Set##AXIS##Tics(Args&& ...args);\
};\
template <class GPM>\
inline void GPM##AXIS##Axis<GPM>::Set##AXIS##Label(const std::string& label) { this->SetLabel(axis, label); }\
template <class GPM>\
inline void GPM##AXIS##Axis<GPM>::Set##AXIS##Range(double min, double max) { this->SetRange(axis, min, max); }\
template <class GPM>\
inline void GPM##AXIS##Axis<GPM>::Set##AXIS##RangeMin(double min) { this->SetRangeMin(axis, min); }\
template <class GPM>\
inline void GPM##AXIS##Axis<GPM>::Set##AXIS##RangeMax(double max) { this->SetRangeMax(axis, max); }\
template <class GPM>\
inline void GPM##AXIS##Axis<GPM>::SetLog##AXIS(double base) { this->SetLog(axis, base); }\
template <class GPM>\
template <class ...Args>\
inline void GPM##AXIS##Axis<GPM>::Set##AXIS##Tics(Args&& ...args) { this->SetTics(axis, std::forward<Args>(args)...); }

DEF_GPMAXIS(X, "x")
DEF_GPMAXIS(X2, "x2")
DEF_GPMAXIS(Y, "y")
DEF_GPMAXIS(Y2, "y2")
DEF_GPMAXIS(Z, "z")
DEF_GPMAXIS(Z2, "z2")
DEF_GPMAXIS(CB, "cb")

#undef DEF_GPMAXIS

template <class GPM>
struct GPM2DAxis : public GPMXAxis<GPMX2Axis<GPMYAxis<GPMY2Axis<GPMCBAxis<GPM>>>>>
{
	using Base = GPMXAxis<GPMX2Axis<GPMYAxis<GPMY2Axis<GPMCBAxis<GPM>>>>>;
	using Base::Base;
};
template <class GPM>
struct GPM3DAxis : public GPMZAxis<GPM2DAxis<GPM>>
{
	using Base = GPMZAxis<GPM2DAxis<GPM>>;
	using Base::Base;
};

}

namespace plot
{

struct BaseOption {};
struct LineOption : public BaseOption {};
struct PointOption : public LineOption {};
struct VectorOption : public LineOption {};
struct FillOption : public BaseOption {};
struct FilledCurveOption : public FillOption {};

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(x, ArrayData, BaseOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(y, ArrayData, BaseOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(title, const std::string&, BaseOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(axis, const std::string&, BaseOption)//y2軸を使いたい場合などに、"x1y2"のように指定する。

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(linetype, int, LineOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(linewidth, double, LineOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(color, const std::string&, LineOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(variable_color, ArrayData, LineOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(style, Style, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(pointtype, int, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(pointsize, double, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(smooth, Smooth, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xerrorbar, ArrayData, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(yerrorbar, ArrayData, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(variable_size, ArrayData, PointOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xlen, ArrayData, VectorOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(ylen, ArrayData, VectorOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(arrowhead, int, VectorOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(fillpattern, int, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(fillsolid, double, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(filltransparent, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(fillcolor, const std::string&, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(variable_fillcolor, const std::string&, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(bordercolor, const std::string&, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(bordertype, int, FillOption)//linetype

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(ybelow, ArrayData, FilledCurveOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(baseline, const std::string&, FilledCurveOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(closed, FilledCurveOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(above, FilledCurveOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(below, FilledCurveOption)

}

namespace detail
{

namespace plot = gpm2::plot;

struct GPMPointParam
{
	GPMPointParam()
		: mLineType(-2), mLineWidth(-1),
		mStyle(Style::none), mPointType(-1), mPointSize(-1), mSmooth(Smooth::none)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		if (KeywordExists(plot::xerrorbar, ops...)) mXErrorbar = GetKeywordArg(plot::xerrorbar, ops...);
		if (KeywordExists(plot::yerrorbar, ops...)) mYErrorbar = GetKeywordArg(plot::yerrorbar, ops...);
		if (KeywordExists(plot::variable_color, ops...)) mVariableColor = GetKeywordArg(plot::variable_color, ops...);
		if (KeywordExists(plot::variable_size, ops...)) mVariableSize = GetKeywordArg(plot::variable_size, ops...);
		mStyle = GetKeywordArg(plot::style, ops..., Style::points);
		mSmooth = GetKeywordArg(plot::smooth, ops..., Smooth::none);
		mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
		mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
		mLineType = GetKeywordArg(plot::linetype, ops..., -2);
		mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
		mColor = GetKeywordArg(plot::color, ops..., "");
	}

	//LineOption
	int mLineType;//-2ならデフォルト
	double mLineWidth;//-1ならデフォルト、-2ならvariable
	std::string mColor;
	plot::ArrayData mVariableColor;

	//PointOption
	Style mStyle;//デフォルトではPOINTS
	int mPointType;//-1ならデフォルト
	double mPointSize;//-1ならデフォルト、-2ならvariable
	Smooth mSmooth;
	plot::ArrayData mX;
	plot::ArrayData mY;
	plot::ArrayData mXErrorbar;
	plot::ArrayData mYErrorbar;
	plot::ArrayData mVariableSize;
};
struct GPMVectorParam
{
	GPMVectorParam()
		: mLineType(-2), mLineWidth(-1), mArrowHead(0)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		mLineType = GetKeywordArg(plot::linetype, ops..., -2);
		mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
		mColor = GetKeywordArg(plot::color, ops..., "");
		if (KeywordExists(plot::variable_color, ops...)) mVariableColor = GetKeywordArg(plot::variable_color, ops...);
	}

	//LineOption
	int mLineType;//-2ならデフォルト
	double mLineWidth;//-1ならデフォルト、-2ならvariable
	std::string mColor;
	plot::ArrayData mVariableColor;

	//VectorOption
	plot::ArrayData mX;
	plot::ArrayData mY;
	plot::ArrayData mXLen;
	plot::ArrayData mYLen;
	int mArrowHead;//-1ならデフォルト。
};
struct GPMFilledCurveParam
{
	GPMFilledCurveParam()
		: mPattern(-1), mSolid(-1), mTransparent(false), mBorderType(-3),
		mClosed(false), mAbove(false), mBelow(false)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		mClosed = GetKeywordArg(plot::closed, ops..., false);
		mAbove = GetKeywordArg(plot::above, ops..., false);
		mBelow = GetKeywordArg(plot::below, ops..., false);
		mFillColor = GetKeywordArg(plot::fillcolor, ops..., "");
		if (KeywordExists(plot::variable_fillcolor, ops...)) mVariableColor = GetKeywordArg(plot::variable_fillcolor, ops...);
		mPattern = GetKeywordArg(plot::fillpattern, ops..., -1);
		mSolid = GetKeywordArg(plot::fillsolid, ops..., -1.);
		mTransparent = KeywordExists(plot::filltransparent, ops...);
		mBorderColor = GetKeywordArg(plot::bordercolor, ops..., "");
		mBorderType = GetKeywordArg(plot::bordertype, ops..., -3);
	}

	//FillOption
	std::string mFillColor;
	plot::ArrayData mVariableColor;
	std::string mBaseline;
	int mPattern;
	double mSolid;
	bool mTransparent;
	std::string mBorderColor;
	int mBorderType;//-2はnorborderを意味する。

	//FilledCurveOption
	plot::ArrayData mX;
	plot::ArrayData mY;
	plot::ArrayData mY2;
	bool mClosed;
	bool mAbove;
	bool mBelow;
};

template <class ...Styles>
struct GPMGraphParamBase : public Variant<Styles...>
{
	GPMGraphParamBase()
		: mType(EQUATION)
	{}
	virtual ~GPMGraphParamBase() = default;

	template <class ...Ops>
	void SetBaseOptions(Ops ...ops)
	{
		mTitle = GetKeywordArg(plot::title, ops..., "");
		mAxis = GetKeywordArg(plot::axis, ops..., "");
	}

	enum Type : char { EQUATION, FILE, DATA, };
	char mType;
	std::string mGraph;

	//Baseoption
	std::string mTitle;
	std::string mAxis;

	std::vector<std::string> mColumn;
};
struct GPMGraphParam2D : public GPMGraphParamBase<GPMPointParam, GPMVectorParam, GPMFilledCurveParam>
{
	void AssignPoint() { Emplace<GPMPointParam>(); }
	void AssignVector() { Emplace<GPMVectorParam>(); }
	void AssignFilledCurve() { Emplace<GPMFilledCurveParam>(); }

	bool IsPoint() const { return Is<GPMPointParam>(); }
	bool IsVector() const { return Is<GPMVectorParam>(); }
	bool IsFilledCurve() const { return Is<GPMFilledCurveParam>(); }

	GPMPointParam& GetPointParam() { return Get<GPMPointParam>(); }
	const GPMPointParam& GetPointParam() const { return Get<GPMPointParam>(); }
	GPMVectorParam& GetVectorParam() { return Get<GPMVectorParam>(); }
	const GPMVectorParam& GetVectorParam() const { return Get<GPMVectorParam>(); }
	GPMFilledCurveParam& GetFilledCurveParam() { return Get<GPMFilledCurveParam>(); }
	const GPMFilledCurveParam& GetFilledCurveParam() const { return Get<GPMFilledCurveParam>(); }
};


template <class GraphParam>
struct GPMPlotBuffer2D
{
	GPMPlotBuffer2D(GPMCanvas* g);
	GPMPlotBuffer2D(const GPMPlotBuffer2D&) = delete;
	GPMPlotBuffer2D(GPMPlotBuffer2D&& p) noexcept;
	GPMPlotBuffer2D& operator=(const GPMPlotBuffer2D&) = delete;
	GPMPlotBuffer2D& operator=(GPMPlotBuffer2D&& p) noexcept;
	virtual ~GPMPlotBuffer2D();

	void Flush();

	template <class Type1, class Type2, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(const std::string& equation, Options ...ops);

	template <class Type1, class Type2, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(const std::string& equation, Options ...ops);

	template <class Type1, class Type2, class Type3, class Type4, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	GPMPlotBuffer2D PlotVectors(const std::vector<Type1>& xfrom, const std::vector<Type2>& yfrom,
								const std::vector<Type3>& xlen, const std::vector<Type4>& ylen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	GPMPlotBuffer2D PlotVectors(const std::string& filename,
								const std::string& xbegin, const std::string& xlen,
								const std::string& ybegin, const std::string& ylen,
								Options ...ops);

	template <class Type1, class Type2, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, Options ...ops);
	template <class Type1, class Type2, class Type3, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, const std::vector<Type3>& y2,
									 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, const std::string& y2,
									 Options ...ops);

protected:

	GPMPlotBuffer2D Plot(GraphParam& i);

	static std::string PlotCommand(const GraphParam& i, const bool IsInMemoryDataTransferEnabled);
	static std::string InitCommand();

	std::vector<GraphParam> mParam;
	GPMCanvas* mCanvas;
};

template <class GraphParam, template <class> class Buffer>
class GPMCanvas2D : public GPM2DAxis<GPMCanvas>
{
public:

	using _Buffer = Buffer<GraphParam>;
	using GPM2DAxis<GPMCanvas>::GPM2DAxis;

	friend class gpm2::GPMMultiPlot;

	template <class Type1, class Type2, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::string& equation, Options ...ops);
	
	template <class Type1, class Type2, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(const std::string& equation, Options ...ops);

	template <class Type1, class Type2, class Type3, class Type4, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	_Buffer PlotVectors(const std::vector<Type1>& xbegin, const std::vector<Type2>& ybegin,
						const std::vector<Type3>& xlen, const std::vector<Type4>& ylen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	_Buffer PlotVectors(const std::string& filename,
						const std::string& xbegin, const std::string& xlen,
						const std::string& ybegin, const std::string& ylen,
						Options ...ops);

	template <class Type1, class Type2, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, Options ...ops);
	template <class Type1, class Type2, class Type3, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, const std::vector<Type3>& y2,
							 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, const std::string& y2,
							 Options ...ops);

	_Buffer GetBuffer();

};

template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam>::GPMPlotBuffer2D(GPMCanvas* g)
	: mCanvas(g) {}
template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam>::GPMPlotBuffer2D(GPMPlotBuffer2D&& p) noexcept
	: mParam(std::move(p.mParam)), mCanvas(p.mCanvas)
{
	p.mCanvas = nullptr;
}
template <class GraphParam>
GPMPlotBuffer2D<GraphParam>& GPMPlotBuffer2D<GraphParam>::operator=(GPMPlotBuffer2D<GraphParam>&& p) noexcept
{
	mCanvas = p.mCanvas; p.mCanvas = nullptr;
	mParam = std::move(p.mParam);
	return *this;
}
template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam>::~GPMPlotBuffer2D()
{
	//mCanvasがnullptrでないときはこのPlotBufferが最終処理を担当する。
	if (mCanvas != nullptr) Flush();
}
template <class GraphParam>
inline void GPMPlotBuffer2D<GraphParam>::Flush()
{
	if (mCanvas == nullptr) throw NotInitialized("Buffer is empty");
	std::string c = "plot";
	for (auto& i : mParam)
	{
		c += PlotCommand(i, mCanvas->IsInMemoryDataTransferEnabled()) + ", ";
	}
	c.erase(c.end() - 2, c.end());
	mCanvas->Command(c);
	mCanvas->Command(InitCommand());
}
template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::Plot(GraphParam& i)
{
	if (i.mType == GraphParam::DATA)
	{
		if (mCanvas->IsInMemoryDataTransferEnabled())
		{
			i.mGraph = "$" + SanitizeForDataBlock(mCanvas->GetOutput()) + "_" + std::to_string(mParam.size()); // datablock name
		}
		else
		{
			i.mGraph = mCanvas->GetOutput() + ".tmp" + std::to_string(mParam.size()) + ".txt";
		}
		auto GET_ARRAY = [](plot::ArrayData& X, const std::string& x,
							std::vector<DataIterator>& it, std::vector<std::string>& column, std::string& labelcolumn, size_t& size)
		{
			switch (X.GetType())
			{
			case plot::ArrayData::DBLVEC:
				if (size == 0) size = X.GetVector().size();
				else if (size != X.GetVector().size()) throw InvalidArg("The number of " + x + " does not match with the others.");
				it.emplace_back(X.GetVector().begin());
				column.emplace_back(std::to_string(it.size()));
				break;
			case plot::ArrayData::STRVEC:
				if (size == 0) size = X.GetStrVec().size();
				else if (size != X.GetStrVec().size()) throw InvalidArg("The number of " + x + " does not match with the others.");
				it.emplace_back(X.GetStrVec().begin());
				labelcolumn = "xtic(" + std::to_string(it.size()) + ")";
				break;
			case plot::ArrayData::COLUMN:
				column.emplace_back(X.GetColumn());
				break;
			case plot::ArrayData::UNIQUE:
				column.emplace_back("($1-$1+" + std::to_string(X.GetValue()) + ")");
				break;
			}
		};

		//std::vector<std::vector<double>::const_iterator> it;
		std::vector<DataIterator> it;
		std::vector<std::string> column;
		std::string labelcolumn;
		size_t size = 0;

		//ファイルを作成する。
		if (i.IsPoint())
		{
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");

			GET_ARRAY(p.mX, "x", it, column, labelcolumn, size);
			GET_ARRAY(p.mY, "y", it, column, labelcolumn, size);

			if (p.mXErrorbar) GET_ARRAY(p.mXErrorbar, "xerrorbar", it, column, labelcolumn, size);
			if (p.mYErrorbar) GET_ARRAY(p.mYErrorbar, "yerrorbar", it, column, labelcolumn, size);
			if (p.mVariableColor) GET_ARRAY(p.mVariableColor, "variable_color", it, column, labelcolumn, size);
			if (p.mVariableSize) GET_ARRAY(p.mVariableSize, "variable_size", it, column, labelcolumn, size);
		}
		else if (i.IsVector())
		{
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(v.mX, "x", it, column, labelcolumn, size);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(v.mY, "y", it, column, labelcolumn, size);
			if (!v.mXLen) throw InvalidArg("xlen list is not given.");
			GET_ARRAY(v.mXLen, "xlen", it, column, labelcolumn, size);
			if (!v.mYLen) throw InvalidArg("ylen list is not given.");
			GET_ARRAY(v.mYLen, "ylen", it, column, labelcolumn, size);

			if (v.mVariableColor) GET_ARRAY(v.mVariableColor, "variable_color", it, column, labelcolumn, size);
		}
		else if (i.IsFilledCurve())
		{
			auto& f = i.GetFilledCurveParam();
			if (!f.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(f.mX, "x", it, column, labelcolumn, size);
			if (!f.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(f.mY, "y", it, column, labelcolumn, size);

			if (f.mY2) GET_ARRAY(f.mY2, "y2", it, column, labelcolumn, size);
			if (f.mVariableColor) GET_ARRAY(f.mVariableColor, "variable_fillcolor", it, column, labelcolumn, size);
		}
		MakeDataObject(mCanvas, i.mGraph, it, size);
		if (!labelcolumn.empty()) column.emplace_back(std::move(labelcolumn));
		i.mColumn = std::move(column);
	}
	else if (i.mType == GraphParam::FILE)
	{
		auto ADD_COLUMN = [](const plot::ArrayData& a, const std::string& name, std::vector<std::string>& c)
		{
			if (a.GetType() == plot::ArrayData::COLUMN) c.emplace_back(a.GetColumn());
			else if (a.GetType() == plot::ArrayData::UNIQUE) c.emplace_back("($1-$1+" + std::to_string(a.GetValue()) + ")");
			else throw InvalidArg(name + "list in the file plot mode must be given in the form of the string column or unique value.");
		};
		//ファイルからプロットする場合、x軸ラベルを文字列にするのがちょっと難しい。
		//普通ならusing 1:2で済むところ、x軸を文字列ラベルにするとusing 2:xticlabels(1)のようにしなければならない。
		//しかしそのためには、ファイルをコメントを除く数行読んでみて、数値か文字列かを判定する必要が生じる。
		//ややこしくなるので今のところ非対応。
		//一応、ユーザー側でPlotPointsの引数に"2", "xticlabels(1)"と与えることで動作はするが、その場合エラーバーなどは使えなくなる。
		std::vector<std::string> column;
		if (i.IsPoint())
		{
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			ADD_COLUMN(p.mX, "x", column);
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");
			ADD_COLUMN(p.mY, "y", column);
			if (p.mXErrorbar) ADD_COLUMN(p.mXErrorbar, "xerrorbar", column);
			if (p.mYErrorbar) ADD_COLUMN(p.mYErrorbar, "yerrorbar", column);
			if (p.mVariableColor) ADD_COLUMN(p.mVariableColor, "variable_color", column);
			if (p.mVariableSize) ADD_COLUMN(p.mVariableSize, "variable_size", column);
		}
		else if (i.IsVector())
		{
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			ADD_COLUMN(v.mX, "x", column);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			ADD_COLUMN(v.mY, "y", column);
			if (!v.mXLen) throw InvalidArg("xlen list is not given.");
			ADD_COLUMN(v.mXLen, "xlen", column);
			if (!v.mYLen) throw InvalidArg("ylen list is not given.");
			ADD_COLUMN(v.mYLen, "ylen", column);
			if (v.mVariableColor) ADD_COLUMN(v.mVariableColor, "variable_color", column);
		}
		else if (i.IsFilledCurve())
		{
			auto& f = i.GetFilledCurveParam();
			if (!f.mX) throw InvalidArg("x coordinate list is not given.");
			ADD_COLUMN(f.mX, "x", column);
			if (!f.mY) throw InvalidArg("y coordinate list is not given.");
			ADD_COLUMN(f.mY, "y", column);

			if (f.mY2) ADD_COLUMN(f.mY2, "y2", column);
			if (f.mVariableColor) ADD_COLUMN(f.mVariableColor, "variable_fillcolor", column);
		}
		i.mColumn = std::move(column);
	}
	mParam.emplace_back(std::move(i));
	return std::move(*this);
}
template <class GraphParam>
template <class Type1, class Type2, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotPoints(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//point
	auto& p = i.GetPointParam();
	p.mX = x;
	p.mY = y;
	p.SetOptions(ops...);

	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotPoints(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mGraph = filename;
	i.mType = GraphParam::FILE;
	i.SetBaseOptions(ops...);

	//point
	auto& p = i.GetPointParam();
	p.mX = xcol;
	p.mY = ycol;
	p.SetOptions(ops...);

	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotPoints(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mGraph = equation;
	i.mType = GraphParam::EQUATION;
	i.SetBaseOptions(ops...);

	//point
	auto& p = i.GetPointParam();
	p.SetOptions(ops...);

	return Plot(i);
}
template <class GraphParam>
template <class Type1, class Type2, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops)
{
	return PlotPoints(x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops)
{
	return PlotPoints(filename, xcol, ycol, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(const std::string& equation, Options ...ops)
{
	return PlotPoints(equation, plot::style = Style::lines, ops...);
}

template <class GraphParam>
template <class Type1, class Type2, class Type3, class Type4, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotVectors(const std::vector<Type1>& xfrom, const std::vector<Type2>& yfrom,
			const std::vector<Type3>& xlen, const std::vector<Type4>& ylen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mXLen = xlen;
	v.mYLen = ylen;
	v.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotVectors(const std::string& filename,
			const std::string& xfrom, const std::string& xlen,
			const std::string& yfrom, const std::string& ylen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mXLen = xlen;
	v.mYLen = ylen;
	v.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
template <class Type1, class Type2, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops)
{
	GraphParam p;
	p.AssignFilledCurve();
	p.mType = GraphParam::DATA;
	p.SetBaseOptions(ops...);

	auto& f = p.GetFilledCurveParam();
	f.mX = x;
	f.mY = y;
	f.SetOptions(ops...);
	return Plot(p);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, Options ...ops)
{
	GraphParam p;
	p.AssignFilledCurve();
	p.mType = GraphParam::FILE;
	p.mGraph = filename;
	p.SetBaseOptions(ops...);

	auto& f = p.GetFilledCurveParam();
	f.mX = x;
	f.mY = y;
	f.SetOptions(ops...);
	return Plot(p);
}
template <class GraphParam>
template <class Type1, class Type2, class Type3, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, const std::vector<Type3>& y2,
				 Options ...ops)
{
	GraphParam p;
	p.AssignFilledCurve();
	p.mType = GraphParam::DATA;
	p.SetBaseOptions(ops...);

	auto& f = p.GetFilledCurveParam();
	f.mX = x;
	f.mY = y;
	f.mY2 = y2;
	f.SetOptions(ops...);
	return Plot(p);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, const std::string& y2, Options ...ops)
{
	GraphParam p;
	p.AssignFilledCurve();
	p.mType = GraphParam::FILE;
	p.mGraph = filename;
	p.SetBaseOptions(ops...);

	auto& f = p.GetFilledCurveParam();
	f.mX = x;
	f.mY = y;
	f.mY2 = y2;
	f.SetOptions(ops...);
	return Plot(p);
}

template <class GraphParam>
inline std::string GPMPlotBuffer2D<GraphParam>::PlotCommand(const GraphParam& p, const bool IsInMemoryDataTransferEnabled)
{
	//filename or equation
	std::string c;
	switch (p.mType) 
	{
	case GraphParam::EQUATION:
		//equation
		c += " " + p.mGraph;
		break;
	case GraphParam::FILE:
		//filename
		c += " '" + p.mGraph + "'";

		//using
		c += " using ";
		for (size_t i = 0; i < p.mColumn.size(); ++i)
			c += p.mColumn[i] + ":";
		c.pop_back();
		break;
	case GraphParam::DATA:
		if (IsInMemoryDataTransferEnabled) 
		{
			//variable name
			c += " " + p.mGraph;
		}
		else {
			//filename
			c += " '" + p.mGraph + "'";
		}

		//using
		c += " using ";
		for (size_t i = 0; i < p.mColumn.size(); ++i)
			c += p.mColumn[i] + ":";
		c.pop_back();
		break;
	}

	//title
	if (!p.mTitle.empty())
	{
		if (p.mTitle == "notitle") c += " notitle";
		else c += " title '" + p.mTitle + "'";
	}

	c += " with";
	//ベクトル指定がある場合。
	if (p.IsVector())
	{
		c += VectorPlotCommand(p.GetVectorParam());
	}
	//点指定がある場合。
	else if (p.IsPoint())
	{
		c += PointPlotCommand(p.GetPointParam());
	}
	else if (p.IsFilledCurve())
	{
		c += FilledCurveplotCommand(p.GetFilledCurveParam());
	}

	//axis
	if (!p.mAxis.empty()) c += " axes " + p.mAxis;

	return c;
}
template <class GraphParam>
inline std::string GPMPlotBuffer2D<GraphParam>::InitCommand()
{
	std::string c;
	c += "set autoscale\n";
	c += "unset logscale\n";
	c += "unset title\n";
	c += "unset grid\n";
	c += "set size noratio\n";
	return c;
}

template <class GraphParam, template <class> class Buffer>
template <class Type1, class Type2, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(filename, xcol, ycol, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(const std::string& equation, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class Type1, class Type2, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(const std::string& filename, const std::string& xcol, const std::string& ycol, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(filename, xcol, ycol, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(const std::string& equation, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class Type1, class Type2, class Type3, class Type4, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotVectors(const std::vector<Type1>& xfrom, const std::vector<Type2>& yfrom,
			const std::vector<Type3>& xlen, const std::vector<Type4>& ylen,
			Options ...ops)
{
	_Buffer r(this);
	return r.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotVectors(const std::string& filename,
			const std::string& xfrom, const std::string& yfrom,
			const std::string& xlen, const std::string& ylen,
			Options ...ops)
{
	_Buffer r(this);
	return r.PlotVectors(filename, xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class Type1, class Type2, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(filename, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class Type1, class Type2, class Type3, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(const std::vector<Type1>& x, const std::vector<Type2>& y, const std::vector<Type3>& y2,
				 Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(x, y, y2, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(const std::string& filename, const std::string& x, const std::string& y, const std::string& y2, Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(filename, x, y, y2, ops...);
}
template <class GraphParam, template <class> class Buffer>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::GetBuffer()
{
	return _Buffer(this);
}

}

enum class Contour { none, base, surface, both, };
enum class CntrSmooth { none, linear, cubicspline, bspline };

namespace plot
{

struct Base3DOption : public BaseOption {};
struct Point3DOption : public PointOption, public Base3DOption {};
struct Vector3DOption : public VectorOption, public Base3DOption {};
struct ColormapOption : public BaseOption {};

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(z, ArrayData, Base3DOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(zlen, ArrayData, Vector3DOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(map, const MatrixData&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xcoord, const ArrayData&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(ycoord, const ArrayData&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xrange, CUF_TIE_ARGS(std::pair<double, double>), ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(yrange, CUF_TIE_ARGS(std::pair<double, double>), ColormapOption)

//options for contour plot
CUF_DEFINE_TAGGED_KEYWORD_OPTION(with_contour, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(without_surface, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrsmooth, CntrSmooth, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrpoints, int, ColormapOption)//the number of lines for cspline and bspline
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrorder, int, ColormapOption)//order for bspline, [2, 10]
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrlevels_auto, int, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrlevels_discrete, const std::vector<double>&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrlevels_incremental, CUF_TIE_ARGS(std::tuple<double, double, double>), ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrcolor, const std::string&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(variable_cntrcolor, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrlinetype, int, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrlinewidth, double, ColormapOption)

}

namespace detail
{

struct GPMPointParamCM : public GPMPointParam
{
	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		GPMPointParam::SetOptions(ops...);
	}
	plot::ArrayData mZ;
};
struct GPMVectorParamCM : public GPMVectorParam
{
	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		GPMVectorParam::SetOptions(ops...);
	}
	plot::ArrayData mZ;
	plot::ArrayData mZLen;
};
struct GPMFilledCurveParamCM : public GPMFilledCurveParam
{
	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		GPMFilledCurveParam::SetOptions(ops...);
	}
	plot::ArrayData mZ;
};
struct GPMColormapParam
{
	GPMColormapParam()
		: mXRange(DBL_MAX, -DBL_MAX), mYRange(DBL_MAX, -DBL_MAX),
		mWithContour(false), mWithoutSurface(false), mCntrSmooth(CntrSmooth::none),
		mCntrPoints(-1), mCntrOrder(-1), mCntrLevelsAuto(-1), mCntrLevelsIncremental(0, 0, 0),
		mVariableCntrColor(false), mCntrLineType(-2), mCntrLineWidth(-1.)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		mWithContour = KeywordExists(plot::with_contour, ops...);
		mWithoutSurface = KeywordExists(plot::without_surface, ops...);
		mCntrSmooth = GetKeywordArg(plot::cntrsmooth, ops..., CntrSmooth::none);
		mCntrPoints = GetKeywordArg(plot::cntrpoints, ops..., -1);
		mCntrOrder = GetKeywordArg(plot::cntrorder, ops..., -1);
		mCntrLevelsAuto = GetKeywordArg(plot::cntrlevels_auto, ops..., -1);
		mCntrLevelsDiscrete = GetKeywordArg(plot::cntrlevels_discrete, ops..., std::vector<double>{});
		mCntrLevelsIncremental = GetKeywordArg(plot::cntrlevels_incremental, ops..., std::tuple<double, double, double>(0., 0., 0.));
		mCntrColor = GetKeywordArg(plot::cntrcolor, ops..., "");
		mVariableCntrColor = KeywordExists(plot::variable_cntrcolor, ops...);
		mCntrLineType = GetKeywordArg(plot::cntrlinetype, ops..., -2);
		mCntrLineWidth = GetKeywordArg(plot::cntrlinewidth, ops..., -1.);
	}
	//ColormapOption
	plot::ArrayData mXCoord;
	plot::ArrayData mYCoord;
	std::pair<double, double> mXRange;
	std::pair<double, double> mYRange;
	plot::MatrixData mZMap;

	bool mWithContour;
	bool mWithoutSurface;
	CntrSmooth mCntrSmooth;
	int mCntrPoints;
	int mCntrOrder;
	int mCntrLevelsAuto;
	std::vector<double> mCntrLevelsDiscrete;
	std::tuple<double, double, double> mCntrLevelsIncremental;
	std::string mCntrColor;
	bool mVariableCntrColor;
	int mCntrLineType;
	double mCntrLineWidth;
};

template <class PointParam, class VectorParam, class FilledCurveParam, class ColormapParam>
struct GPMGraphParamBaseCM
	: public GPMGraphParamBase<PointParam, VectorParam, FilledCurveParam, ColormapParam>
{
	template <class ...Ops>
	void SetBaseOptions(Ops ...ops)
	{
		GPMGraphParamBase<PointParam, VectorParam, FilledCurveParam, ColormapParam>::SetBaseOptions(ops...);
	}

	void AssignPoint() { this->template Emplace<PointParam>(); }
	void AssignVector() { this->template Emplace<VectorParam>(); }
	void AssignFilledCurve() { this->template Emplace<FilledCurveParam>(); }
	void AssignColormap() { this->template Emplace<ColormapParam>(); }

	bool IsPoint() const { return this->template Is<PointParam>(); }
	bool IsVector() const { return this->template Is<VectorParam>(); }
	bool IsFilledCurve() const { return this->template Is<FilledCurveParam>(); }
	bool IsColormap() const { return this->template Is<ColormapParam>(); }

	PointParam& GetPointParam() { return this->template Get<PointParam>(); }
	const PointParam& GetPointParam() const { return this->template Get<PointParam>(); }
	VectorParam& GetVectorParam() { return this->template Get<VectorParam>(); }
	const VectorParam& GetVectorParam() const { return this->template Get<VectorParam>(); }
	FilledCurveParam& GetFilledCurveParam() { return this->template Get<FilledCurveParam>(); }
	const FilledCurveParam& GetFilledCurveParam() const { return this->template Get<FilledCurveParam>(); }
	ColormapParam& GetColormapParam() { return this->template Get<ColormapParam>(); }
	const ColormapParam& GetColormapParam() const { return this->template Get<ColormapParam>(); }
};

using GPMGraphParamCM = GPMGraphParamBaseCM<GPMPointParamCM, GPMVectorParamCM, GPMFilledCurveParamCM, GPMColormapParam>;

template <class GraphParam>
struct GPMPlotBufferCM
{
	GPMPlotBufferCM(GPMCanvas* g);
	GPMPlotBufferCM(const GPMPlotBufferCM&) = delete;
	GPMPlotBufferCM(GPMPlotBufferCM&& p) noexcept;
	GPMPlotBufferCM& operator=(const GPMPlotBufferCM&) = delete;
	GPMPlotBufferCM& operator=(GPMPlotBufferCM&& p) noexcept;
	virtual ~GPMPlotBufferCM();

	void Flush();

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::string& filename,
							   const std::string& x, const std::string& y, const std::string& z,
							   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::string& filename,
							   const std::string& x, const std::string& y,
							   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::string& filename,
							  const std::string& x, const std::string& y, const std::string& z,
							  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::string& filename,
							  const std::string& x, const std::string& y,
							  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
								const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(const std::string& filename,
								const std::string& xfrom, const std::string& yfrom, const std::string& zfrom,
								const std::string& xlen, const std::string& ylen, const std::string& zlen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
								const std::vector<double>& xlen, const std::vector<double>& ylen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(const std::string& filename,
								const std::string& xfrom, const std::string& yfrom,
								const std::string& xlen, const std::string& ylen,
								Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
								 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
								 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const std::string& filename, const std::string& z, const std::string& x, const std::string& y,
								 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const std::string& equation, Options ...ops);

protected:

	GPMPlotBufferCM Plot(GraphParam& i);

	static std::string PlotCommand(const GraphParam& i, const bool IsInMemoryDataTransferEnabled);
	static std::string InitCommand();

	std::vector<GraphParam> mParam;
	GPMCanvas* mCanvas;
};


template <class GraphParam, template <class> class Buffer>
class GPMCanvasCM : public GPM2DAxis<GPMCanvas>
{
public:

	using _Buffer = Buffer<GraphParam>;

	GPMCanvasCM(const std::string& output, double sizex = 0., double sizey = 0.);
	GPMCanvasCM();

	friend class gpm2::GPMMultiPlot;

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::string& filename,
					   const std::string& x, const std::string& y, const std::string& z,
					   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::string& filename,
					   const std::string& x, const std::string& y,
					   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::string& filename,
					  const std::string& x, const std::string& y, const std::string& z,
					  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::string& filename,
					  const std::string& x, const std::string& y,
					  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
						const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(const std::string& filename,
						const std::string& xfrom, const std::string& yfrom, const std::string& zfrom,
						const std::string& xlen, const std::string& ylen, const std::string& zlen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
						const std::vector<double>& xlen, const std::vector<double>& ylen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(const std::string& filename,
						const std::string& xfrom, const std::string& yfrom,
						const std::string& xlen, const std::string& ylen,
						Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
						 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
						 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const std::string& filename, const std::string& z, const std::string& x, const std::string& y,
						 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const std::string& equation, Options ...ops);

	_Buffer GetBuffer();
};

template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>::GPMPlotBufferCM(GPMCanvas* g)
	: mCanvas(g) {}
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>::GPMPlotBufferCM(GPMPlotBufferCM&& p) noexcept
	: mParam(std::move(p.mParam)), mCanvas(p.mCanvas)
{
	p.mCanvas = nullptr;
}
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>& GPMPlotBufferCM<GraphParam>::operator=(GPMPlotBufferCM<GraphParam>&& p) noexcept
{
	mCanvas = p.mCanvas; p.mCanvas = nullptr;
	mParam = std::move(p.mParam);
	return *this;
}
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>::~GPMPlotBufferCM()
{
	//mPipeがnullptrでないときはこのPlotterが最終処理を担当する。
	if (mCanvas != nullptr) Flush();
}
template <class GraphParam>
inline void GPMPlotBufferCM<GraphParam>::Flush()
{
	if (mCanvas == nullptr) throw NotInitialized("Buffer is empty");
	std::string c = "splot";
	for (auto& i : mParam)
	{
		c += PlotCommand(i, mCanvas->IsInMemoryDataTransferEnabled()) + ", ";
	}
	c.erase(c.end() - 2, c.end());
	mCanvas->Command(c);
	mCanvas->Command(InitCommand());
}
struct GetCoordFromVector
{
	GetCoordFromVector(const std::vector<double>& x)
		: x(x), size(x.size())
	{
		wmean = (x.back() - x.front()) / (size - 1);
	}
	double operator()(size_t i) const
	{
		if (i == 0) return x[i] - wmean / 2.;
		else if (i == size) return x[i - 1] + wmean / 2.;
		else return (x[i] + x[i - 1]) / 2.;
	}
	double center(size_t i) const
	{
		return i != size ? x[i] : x[i - 1] + wmean;
	}
	const std::vector<double>& x;
	size_t size;
	double wmean;
};
struct GetCoordFromRange
{
	GetCoordFromRange(const std::pair<double, double>& r, size_t size)
	{
		this->size = size;
		width = (r.second - r.first) / (size - 1);
		min = r.first - width / 2.;
		cmin = r.first;
	}
	double operator()(size_t i) const
	{
		return min + i * width;
	}
	double center(size_t i) const
	{
		return cmin + i * width;
	}
	double min;
	double cmin;
	size_t size;
	double width;
};
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::Plot(GraphParam& i)
{
	if (i.mType == GraphParam::DATA)
	{
		if (mCanvas->IsInMemoryDataTransferEnabled())
		{
			i.mGraph = "$" + SanitizeForDataBlock(mCanvas->GetOutput()) + "_" + std::to_string(mParam.size()); // datablock name
		}
		else
		{
			i.mGraph = mCanvas->GetOutput() + ".tmp" + std::to_string(mParam.size()) + ".txt";
		}
		auto GET_ARRAY = [](plot::ArrayData& X, const std::string& x,
							std::vector<DataIterator>& it, std::vector<std::string>& column, std::string& labelcolumn, size_t& size)
		{
			switch (X.GetType())
			{
			case plot::ArrayData::DBLVEC:
				if (size == 0) size = X.GetVector().size();
				else if (size != X.GetVector().size()) throw InvalidArg("The number of " + x + " does not match with the others.");
				it.emplace_back(X.GetVector().begin());
				column.emplace_back(std::to_string(it.size()));
				break;
			case plot::ArrayData::STRVEC:
				if (size == 0) size = X.GetStrVec().size();
				else if (size != X.GetStrVec().size()) throw InvalidArg("The number of " + x + " does not match with the others.");
				it.emplace_back(X.GetStrVec().begin());
				labelcolumn = "xtic(" + std::to_string(it.size()) + ")";
				break;
			case plot::ArrayData::COLUMN:
				column.emplace_back(X.GetColumn());
				break;
			case plot::ArrayData::UNIQUE:
				column.emplace_back("($1-$1+" + std::to_string(X.GetValue()) + ")");
				break;
			}
		};

		std::vector<std::string> column;
		std::string labelcolumn;

		//ファイルを作成する。
		if (i.IsColormap())
		{
			size_t xsize = 0;
			size_t ysize = 0;
			auto& m = i.GetColormapParam();
			if (!m.mZMap) throw InvalidArg("z map is not given");
			if (m.mZMap.GetType() != plot::MatrixData::DBLMAT)
				throw InvalidArg("z map in the data plot mode must be given in the form of Matrix<double>.");

			//mapがMatrixであるとき、x、yの座標値も配列かrangeで与えられていなければならない。
			if (!((m.mXCoord && m.mXCoord.GetType() == plot::ArrayData::DBLVEC) ||
				  m.mXRange != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");
			if (!((m.mYCoord && m.mYCoord.GetType() == plot::ArrayData::DBLVEC) ||
				  m.mYRange != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");

			xsize = m.mZMap.GetMatrix().GetSize(0);
			ysize = m.mZMap.GetMatrix().GetSize(1);

			column = { "1", "2", "5" };
			if (m.mXCoord)
			{
				const auto& x = m.mXCoord.GetVector();
				if (x.size() != xsize) throw InvalidArg("size of x coordinate list and the x size of mat must be the same.");
				if (m.mYCoord)
				{
					const auto& y = m.mYCoord.GetVector();
					if (y.size() != ysize) throw InvalidArg("size of y coordinate list and the y size of mat must be the same.");
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), GetCoordFromVector(x), GetCoordFromVector(y));
				}
				else
				{
					auto y = m.mYRange;
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), GetCoordFromVector(x), GetCoordFromRange(y, ysize));
				}
			}
			else
			{
				auto x = m.mXRange;
				if (m.mYCoord)
				{
					const auto& y = m.mYCoord.GetVector();
					if (y.size() != ysize) throw InvalidArg("size of y coordinate list and the y size of mat must be the same.");
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), GetCoordFromRange(x, xsize), GetCoordFromVector(y));
				}
				else
				{
					auto y = m.mYRange;
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), GetCoordFromRange(x, xsize), GetCoordFromRange(y, ysize));
				}
			}

			//最後にcontourを作成する。
			if (m.mWithContour)
			{
				mCanvas->Command("set contour base");
				if (m.mCntrSmooth != CntrSmooth::none)
				{
					switch (m.mCntrSmooth)
					{
					case CntrSmooth::linear: mCanvas->Command("set cntrparam linear"); break;
					case CntrSmooth::cubicspline: mCanvas->Command("set cntrparam cubicspline"); break;
					case CntrSmooth::bspline: mCanvas->Command("set cntrparam bspline"); break;
					default: break;
					}
				}
				if (m.mCntrPoints != -1) mCanvas->Command(Format("set cntrparam points %d", m.mCntrPoints));
				if (m.mCntrOrder != -1) mCanvas->Command(Format("set cntrparam order %d", m.mCntrOrder));

				if (m.mCntrLevelsAuto != -1)
				{
					mCanvas->Command(Format("set cntrparam levels auto %d", m.mCntrLevelsAuto));
				}
				else if (!m.mCntrLevelsDiscrete.empty())
				{
					std::string str;
					for (auto x : m.mCntrLevelsDiscrete) str += std::to_string(x) + ", ";
					str.erase(str.end() - 2, str.end());
					mCanvas->Command("set cntrparam levels discrete " + str);
				}
				else if (m.mCntrLevelsIncremental != std::tuple<double, double, double>{ 0, 0, 0 })
				{
					double start, incr, end;
					std::tie(start, incr, end) = m.mCntrLevelsIncremental;
					mCanvas->Command(Format("set cntrparam levels incremental %lf, %lf, %lf", start, incr, end));
				}

				mCanvas->Command("set pm3d implicit");
				mCanvas->Command("set contour base");
				mCanvas->Command("unset surface");
				if (mCanvas->IsInMemoryDataTransferEnabled())
				{
					mCanvas->Command("set table " + i.mGraph + "_cntr");
				}
				else {
					std::string path = i.mGraph;
					path.erase(path.end() - 3, path.end());
					path += "cntr.txt";
					mCanvas->Command("set table '" + path + "'");
				}
				//3:4:column[2]でplotする。
				mCanvas->Command(Format("splot '%s' using 3:4:%s t '%s'", i.mGraph, column[2], i.mTitle));
				mCanvas->Command("unset table");
				mCanvas->Command("set surface");
				mCanvas->Command("unset contour");
				mCanvas->Command("unset pm3d");
			}
		}
		else if (i.IsPoint())
		{
			std::vector<DataIterator> it;
			size_t size = 0;
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(p.mX, "x", it, column, labelcolumn, size);
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(p.mY, "y", it, column, labelcolumn, size);
			if (!p.mZ) throw InvalidArg("z coordinate list is not given.");
			GET_ARRAY(p.mZ, "z", it, column, labelcolumn, size);

			if (p.mXErrorbar)
			{
				GET_ARRAY(p.mXErrorbar, "xerrorbar", it, column, labelcolumn, size);
			}
			if (p.mYErrorbar)
			{
				GET_ARRAY(p.mYErrorbar, "yerrorbar", it, column, labelcolumn, size);
			}
			if (p.mVariableColor)
			{
				GET_ARRAY(p.mVariableColor, "variable_color", it, column, labelcolumn, size);
			}
			if (p.mVariableSize)
			{
				GET_ARRAY(p.mVariableSize, "variable_size", it, column, labelcolumn, size);
			}
			MakeDataObject(mCanvas, i.mGraph, it, size);
		}
		else if (i.IsVector())
		{
			std::vector<DataIterator> it;
			size_t size = 0;
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(v.mX, "x", it, column, labelcolumn, size);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(v.mY, "y", it, column, labelcolumn, size);
			if (!v.mZ) throw InvalidArg("z coordinate list is not given.");
			GET_ARRAY(v.mZ, "z", it, column, labelcolumn, size);
			if (!v.mXLen) throw InvalidArg("xlen list is not given.");
			GET_ARRAY(v.mXLen, "xlen", it, column, labelcolumn, size);
			if (!v.mYLen) throw InvalidArg("ylen list is not given.");
			GET_ARRAY(v.mYLen, "ylen", it, column, labelcolumn, size);
			if (!v.mZLen) throw InvalidArg("zlen list is not given.");
			GET_ARRAY(v.mZLen, "zlen", it, column, labelcolumn, size);

			if (v.mVariableColor)
			{
				GET_ARRAY(v.mVariableColor, "variable_color", it, column, labelcolumn, size);
			}
			MakeDataObject(mCanvas, i.mGraph, it, size);
		}
		if (!labelcolumn.empty()) column.emplace_back(std::move(labelcolumn));
		i.mColumn = std::move(column);
	}
	else if (i.mType == GraphParam::FILE)
	{
		auto ADD_COLUMN = [](const plot::ArrayData& a, const std::string& name, std::vector<std::string>& c)
		{
			if (a.GetType() == plot::ArrayData::COLUMN) c.emplace_back(a.GetColumn());
			else if (a.GetType() == plot::ArrayData::UNIQUE) c.emplace_back("($1-$1+" + std::to_string(a.GetValue()) + ")");
			else throw InvalidArg(name + "list in the file plot mode must be given in the form of the string column or unique value.");
		};
		std::vector<std::string> column;
		if (i.IsColormap())
		{
			auto& p = i.GetColormapParam();
			if (!p.mZMap) throw InvalidArg("z coordinate list is not given.");
			if (p.mZMap.GetType() == plot::MatrixData::COLUMN) column.emplace_back(p.mZMap.GetColumn());
			else if (p.mZMap.GetType() == plot::MatrixData::UNIQUE) column.emplace_back("($1-$1+" + std::to_string(p.mZMap.GetValue()) + ")");
			else throw InvalidArg("z coordinate list in the file plot mode must be given in the form of the string column or unique value.");

			if (!p.mXCoord) throw InvalidArg("x coordinate list is not given.");
			ADD_COLUMN(p.mXCoord, "x", column);
			if (!p.mYCoord) throw InvalidArg("y coordinate list is not given.");
			ADD_COLUMN(p.mYCoord, "y", column);
			//既存のファイルからプロットする場合、等高線表示に対応しない。
		}
		else if (i.IsPoint())
		{
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			ADD_COLUMN(p.mX, "x", column);
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");
			ADD_COLUMN(p.mY, "y", column);
			if (!p.mZ) throw InvalidArg("z coordinate list is not given.");
			ADD_COLUMN(p.mZ, "z", column);
			if (p.mXErrorbar) ADD_COLUMN(p.mXErrorbar, "xerrorbar", column);
			if (p.mYErrorbar) ADD_COLUMN(p.mYErrorbar, "yerrorbar", column);
			if (p.mVariableColor) ADD_COLUMN(p.mVariableColor, "variable_color", column);
			if (p.mVariableSize) ADD_COLUMN(p.mVariableSize, "variable_size", column);
		}
		else if (i.IsVector())
		{
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			ADD_COLUMN(v.mX, "x", column);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			ADD_COLUMN(v.mY, "y", column);
			if (!v.mZ) throw InvalidArg("z coordinate list is not given.");
			ADD_COLUMN(v.mZ, "z", column);
			if (!v.mXLen) throw InvalidArg("xlen list is not given.");
			ADD_COLUMN(v.mXLen, "xlen", column);
			if (!v.mYLen) throw InvalidArg("ylen list is not given.");
			ADD_COLUMN(v.mYLen, "ylen", column);
			if (!v.mZLen) throw InvalidArg("zlen list is not given.");
			ADD_COLUMN(v.mZLen, "zlen", column);
			if (v.mVariableColor) ADD_COLUMN(v.mVariableColor, "variable_color", column);
		}
		//colormapはFilledCurve非対応。
		i.mColumn = std::move(column);
	}
	mParam.emplace_back(std::move(i));
	return std::move(*this);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);
	//point
	auto& p = i.GetPointParam();
	p.mX = x;
	p.mY = y;
	p.mZ= z;
	p.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::string& filename,
		   const std::string& x, const std::string& y, const std::string& z,
		   Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	auto& p = i.GetPointParam();
	p.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//point
	auto& p = i.GetPointParam();
	p.mX = x;
	p.mY = y;
	p.mZ = 0.;
	p.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::string& filename,
		   const std::string& x, const std::string& y,
		   Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	auto& p = i.GetPointParam();
	p.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::EQUATION;
	i.SetBaseOptions(ops...);
	i.mGraph = equation;

	auto& p = i.GetPointParam();
	p.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops)
{
	return PlotPoints(x, y, z, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::string& filename,
		  const std::string& x, const std::string& y, const std::string& z,
		  Options ...ops)
{
	return PlotPoints(filename, x, y, z, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	return PlotPoints(x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::string& filename,
		  const std::string& x, const std::string& y,
		  Options ...ops)
{
	return PlotPoints(filename, x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::string& equation, Options ...ops)
{
	return PlotPoints(equation, plot::style = Style::lines, ops...);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//vector
	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mZ = zfrom;
	v.mXLen = xlen;
	v.mYLen = ylen;
	v.mZLen = zlen;
	v.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotVectors(const std::string& filename,
			const std::string& xfrom, const std::string& yfrom, const std::string& zfrom,
			const std::string& xlen, const std::string& ylen, const std::string& zlen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	//vector
	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mZ = zfrom;
	v.mXLen = xlen;
	v.mYLen = ylen;
	v.mZLen = zlen;
	v.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//vector
	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mZ = 0.;
	v.mXLen = xlen;
	v.mYLen = ylen;
	v.mZLen = 0.;
	v.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotVectors(const std::string& filename,
			const std::string& xfrom, const std::string& yfrom,
			const std::string& xlen, const std::string& ylen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	//vector
	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mZ = 0.;
	v.mXLen = xlen;
	v.mYLen = ylen;
	v.mZLen = 0.;
	v.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
			 Options ...ops)
{
	GraphParam i;
	i.AssignColormap();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//map
	auto& m = i.GetColormapParam();
	m.mZMap = map;
	m.mXCoord = x;
	m.mYCoord = y;
	m.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const std::string& filename, const std::string& z, const std::string& x, const std::string& y,
			 Options ...ops)
{
	GraphParam i;
	i.AssignColormap();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	//map
	auto& m = i.GetColormapParam();
	m.mZMap = z;
	m.mXCoord = x;
	m.mYCoord = y;
	m.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.AssignColormap();
	i.mGraph = equation;
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//map
	auto& m = i.GetColormapParam();
	m.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
			 Options ...ops)
{
	GraphParam i;
	i.AssignColormap();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	//map
	auto& m = i.GetColormapParam();
	m.mZMap = map;
	m.mXRange = x;
	m.mYRange = y;
	m.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
inline std::string GPMPlotBufferCM<GraphParam>::PlotCommand(const GraphParam& p, const bool IsInMemoryDataTransferEnabled)
{
	//filename or equation
	std::string c;
	switch (p.mType) 
	{
	case GraphParam::EQUATION:
		//equation
		c += " " + p.mGraph;
		break;
	case GraphParam::FILE:
		//filename
		c += " '" + p.mGraph + "'";

		//using
		c += " using ";
		for (size_t i = 0; i < p.mColumn.size(); ++i)
			c += p.mColumn[i] + ":";
		c.pop_back();
		break;
	case GraphParam::DATA:
		if (IsInMemoryDataTransferEnabled) 
		{
			//variable name
			c += " " + p.mGraph;
		}
		else {
			//filename
			c += " '" + p.mGraph + "'";
		}

		//using
		c += " using ";
		for (size_t i = 0; i < p.mColumn.size(); ++i)
			c += p.mColumn[i] + ":";
		c.pop_back();
		break;
	}

	//title
	if (!p.mTitle.empty())
	{
		if (p.mTitle == "notitle") c += " notitle";
		else c += " title '" + p.mTitle + "'";
	}

	c += " with";
	//カラーマップの場合。
	if (p.IsColormap())
	{
		c += " pm3d";
		auto& m = p.GetColormapParam();
		if (m.mWithoutSurface) c += " nosurface";
	}
	//ベクトルの場合。
	else if (p.IsVector())
	{
		c += VectorPlotCommand(p.GetVectorParam());
	}
	//点の場合。
	else if (p.IsPoint())
	{
		c += PointPlotCommand(p.GetPointParam());
	}

	//axis
	if (!p.mAxis.empty()) c += " axes " + p.mAxis;

	//もしカラーマップでcontourが有効だったら、
	//更にそれを描画するコマンドを追加する。
	if (p.IsColormap())
	{
		auto& m = p.GetColormapParam();
		if (m.mWithContour)
		{
			if (IsInMemoryDataTransferEnabled) 
			{
				c += ", " + p.mGraph + "_cntr with line";
			}
			else
			{
				std::string str = "'" + p.mGraph;
				str.erase(str.end() - 3, str.end());
				c += ", " + str + "cntr.txt' with line";
			}
			if (p.mTitle == "notitle") c += " notitle";
			else c += " title '" + p.mTitle + "'";
			if (m.mCntrLineType != -2) c += Format(" linetype %d", m.mCntrLineType);
			if (m.mCntrLineWidth != -1) c += Format(" linewidth %lf", m.mCntrLineWidth);
			if (m.mVariableCntrColor) c += " linecolor palette";
			else if (!m.mCntrColor.empty()) c += " linecolor '" + m.mCntrColor + "'";
		}
	}
	return c;
}
template <class GraphParam>
inline std::string GPMPlotBufferCM<GraphParam>::InitCommand()
{
	std::string c;
	c += "set autoscale\n";
	c += "unset logscale\n";
	c += "unset title\n";
	c += "unset grid\n";
	c += "set size noratio\n";
	c += "unset pm3d\n";
	c += "unset contour\n";
	c += "set surface";
	return c;
}

template <class GraphParam, template <class> class Buffer>
inline GPMCanvasCM<GraphParam, Buffer>::GPMCanvasCM(const std::string& output, double sizex, double sizey)
	: detail::GPM2DAxis<GPMCanvas>(output, sizex, sizey)
{
	if (mPipe)
	{
		this->Command("set pm3d corners2color c1");
		this->Command("set view map");
	}
}
template <class GraphParam, template <class> class Buffer>
inline GPMCanvasCM<GraphParam, Buffer>::GPMCanvasCM()
{
	if (mPipe)
	{
		this->Command("set pm3d corners2color c1");
		this->Command("set view map");
	}
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(const std::string& filename,
		   const std::string& x, const std::string& y, const std::string& z,
		   Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(filename, x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(const std::string& filename,
		   const std::string& x, const std::string& y,
		   Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(filename, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(const std::string& equation, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(const std::string& filename,
		  const std::string& x, const std::string& y, const std::string& z,
		  Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(filename, x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(const std::string& filename,
		  const std::string& x, const std::string& y,
		  Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(filename, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(const std::string& equation, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(xfrom, yfrom, zfrom, xlen, ylen, zlen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(const std::string& filename,
			const std::string& xfrom, const std::string& yfrom, const std::string& zfrom,
			const std::string& xlen, const std::string& ylen, const std::string& zlen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(filename, xfrom, yfrom, zfrom, xlen, ylen, zlen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(const std::string& filename,
			const std::string& xfrom, const std::string& yfrom,
			const std::string& xlen, const std::string& ylen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(filename, xfrom, yfrom, xlen, ylen, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
			 Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(map, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
			 Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(map, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(const std::string& filename, const std::string& z, const std::string& x, const std::string& y,
			 Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(filename, z, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(const std::string& equation, Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(equation, ops...);
}
template <class GraphParam, template <class> class Buffer>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::GetBuffer()
{
	return _Buffer(this);
}

}

using GPMCanvas2D = detail::GPMCanvas2D<detail::GPMGraphParam2D, detail::GPMPlotBuffer2D>;
using GPMCanvasCM = detail::GPMCanvasCM<detail::GPMGraphParamCM, detail::GPMPlotBufferCM>;

class GPMMultiPlot
{
public:
	GPMMultiPlot(const std::string& outputname, int row, int column, double sizex = 0., double sizey = 0.);
	~GPMMultiPlot();

	void Begin(const std::string& output, int row, int column, double sizex = 0., double sizey = 0.);
	void End();

	void Command(const std::string& c);
private:
};

inline GPMMultiPlot::GPMMultiPlot(const std::string& output, int row, int column, double sizex, double sizey)
{
	Begin(output, row, column, sizex, sizey);
}
inline GPMMultiPlot::~GPMMultiPlot()
{
	End();
}
inline void GPMMultiPlot::Begin(const std::string& output, int row, int column, double sizex, double sizey)
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
			std::string extension = output.substr(output.size() - 4, 4);
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
inline void GPMMultiPlot::Command(const std::string& str)
{
	fprintf(GPMCanvas::Paths<>::msGlobalPipe, "%s\n", str.c_str());
	fflush(GPMCanvas::Paths<>::msGlobalPipe);
}

}

}

#endif
