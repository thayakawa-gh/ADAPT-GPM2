//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#ifndef GPM2_GPMCANVAS_H
#define GPM2_GPMCANVAS_H



#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ADAPT/CUF/Matrix.h>
#include <ADAPT/CUF/KeywordArgs.h>
#include <ADAPT/CUF/Format.h>
#include <ADAPT/GPM2/GPMArrayData.h>

namespace adapt
{

namespace gpm2
{

class GPMMultiPlotter;

class GPMCanvas
{
public:

	friend class GPMMultiPlotter;

	GPMCanvas(const std::string& output);
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
	void SetLog(const std::string& axis, double scale = 10);

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

	void SetGrid(const std::string& color = "", int type = -1, int width = -1);

	void SetSize(double x, double y);
	void SetSizeRatio(double ratio);//-1を与えるとticsの幅が等しくなる。

	void SetPaletteDefined(const std::vector<std::pair<double, std::string>>& color);
	void SetPaletteRGBFormulae(int x, int y, int z);
	void SetPaletteCubehelix(double start, double cycles, double saturation);
	//void SetKeyOff();
	//void SetKey(const std::string& posx, const std::string& posy);//"left", "right", "top", "bottom", "outside", "below"をxyそれぞれに。

	void SetTitle(const std::string& title);

	void SetParametric();


	void SetOutput(const std::string& output);
	void Reset();
	const std::string& GetOutput() const;

	void Command(const std::string& str);

	static void SetGnuplotPath(const std::string& path);
	//GnuplotPathは
	//1.SetGnuplotPathで設定されているなら、その値。
	//2.環境変数GNUPLOT_PATHがあれば、その値。
	//3.windowsの場合、"C:/PROGRA~2/gnuplot/bin/gnuplot.exe"
	//の優先度で取得する。
	static std::string GetGnuplotPath();

protected:

	std::string mOutput;
	FILE* mPipe;
	template <class = void>
	struct Paths
	{
		static std::string msGnuplotPath;
#ifdef _WIN32
		static const std::string msDefaultGnuplotPath;
#endif
		static FILE* msGlobalPipe;//multiplotなどを利用する際のグローバルなパイプ。これがnullptrでない場合、mPipe==mGlobalPipeとなる。
	};
};


inline GPMCanvas::GPMCanvas(const std::string& output)
	: mOutput(output), mPipe(nullptr)
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
			SetOutput(output);
		}
	}
	if (mPipe)
	{
		Command("set bars small");
		Command("set palette defined ( 0 '#000090',1 '#000fff',2 '#0090ff',3 '#0fffee',4 '#90ff70',5 '#ffee00',6 '#ff7000',7 '#ee0000',8 '#7f0000')");
	}
}
inline GPMCanvas::GPMCanvas()
	: mOutput("ADAPT_GPM2_TMPFILE"), mPipe(nullptr)
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

inline void GPMCanvas::SetLog(const std::string& axis, double scale)
{
	Command(Format("set logscale %s %lf", axis, scale));
}

inline void GPMCanvas::SetTics_make(std::string& tics)
{
	tics.erase(tics.end() - 2, tics.end());
	tics += ")";
	//途中。
}

inline void GPMCanvas::SetGrid(const std::string& color, int type, int width)
{
	std::string c;
	if (!color.empty()) c += " linecolor rgb \"" + color + "\"";
	if (type != -1) c += " linetype " + std::to_string(type);
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
inline void GPMCanvas::SetTitle(const std::string& title)
{
	Command("set title '" + title + "'");
}
inline void GPMCanvas::SetParametric()
{
	Command("set parametric");
}
inline void GPMCanvas::SetOutput(const std::string& output)
{
	if (output.size() > 4)
	{
		std::string extension = output.substr(output.size() - 4, 4);
		std::string repout = ReplaceStr(output, "\\", "/");
		if (extension == ".png") Command("set terminal pngcairo enhanced size 800, 600\nset output '" + repout + "'");
		else if (extension == ".eps") Command("set terminal epscairo enhanced size 6in, 4in\nset output '" + repout + "'");
		else if (extension == ".pdf") Command("set terminal pdfcairo enhanced size 6in, 4in\nset output '" + repout + "'");
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

inline void GPMCanvas::Command(const std::string& str)
{
	fprintf(mPipe, (str + "\n").c_str());
	fflush(mPipe);
}


inline void GPMCanvas::SetGnuplotPath(const std::string& path)
{
	Paths<>::msGnuplotPath = path;
}
inline std::string GPMCanvas::GetGnuplotPath()
{
	if (!Paths<>::msGnuplotPath.empty()) return Paths<>::msGnuplotPath;
	if (const char* p = std::getenv("GNUPLOT_PATH")) return std::string(p);
#ifdef _WIN32
	return Paths<>::msDefaultGnuplotPath;
#else
	return std::string();
#endif
}
template <class T>
std::string GPMCanvas::Paths<T>::msGnuplotPath = "";
#ifdef _WIN32
template <class T>
const std::string GPMCanvas::Paths<T>::msDefaultGnuplotPath = "C:/PROGRA~2/gnuplot/bin/gnuplot.exe";
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

inline void MakeFile(std::vector<std::vector<double>::const_iterator>& its, size_t size, const std::string& filename)
{
	std::ofstream ofs(filename);
	if (!ofs) throw InvalidArg("file \"" + filename + "\" cannot open.");
	for (size_t i = 0; i < size; ++i)
	{
		for (auto& it : its)
		{
			ofs << " " << *it;
			++it;
		}
		ofs << "\n";
	}
}
template <class GetX, class GetY>
inline void MakeFile(const Matrix<double>& map, GetX getx, GetY gety, const std::string& filename)
{
	std::ofstream ofs(filename);
	if (!ofs) throw InvalidArg("file \"" + filename + "\" cannot open.");
	uint32_t xsize = map.GetSize(0);
	uint32_t ysize = map.GetSize(1);
	//xsize、ysizeはxcoord.size()-1、ycoord.size()-1にそれぞれ等しいはず。
	for (uint32_t iy = 0; iy < ysize; ++iy)
	{
		double y = gety(iy);
		for (uint32_t ix = 0; ix < xsize; ++ix)
		{
			double x = getx(ix);
			ofs << x << " " << y << " " << map[ix][iy] << "\n";
		}
		double x = getx(xsize);
		ofs << x << " " << y << " 0\n\n";
	}
	double y = gety(ysize);
	for (uint32_t ix = 0; ix < xsize; ++ix)
	{
		double x = getx(ix);
		ofs << x << " " << y << " 0\n";
	}
	ofs << getx(xsize) << " " << y << " 0\n";
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
			if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
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
			if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
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
		}
		if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColor) c += " linecolor palette";
	}
	else if (p.mStyle == Style::points)
	{
		c += " points";
		if (p.mLineType != -1)
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
		if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
		if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
		if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColor) c += " linecolor palette";
	}
	else if (p.mStyle == Style::dots)
	{
		c += " dots";
		if (p.mLineType != -1)
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
		}
	}
	return std::move(c);
}
template<class VectorParam>
std::string VectorPlotCommand(const VectorParam& v)
{
	std::string c;
	c += " vector ";
	if (v.mArrowHead != -1)
	{
		if ((v.mArrowHead & 0b11) == 0) c += " head";
		else if ((v.mArrowHead & 0b11) == 1) c += " heads";
		else if ((v.mArrowHead & 0b11) == 2) c += " noheads";
		if ((v.mArrowHead & 0b1100) == 4) c += " filled";
		else if ((v.mArrowHead & 0b1100) == 5) c += " empty";
		else if ((v.mArrowHead & 0b1100) == 6) c += " nofilled";
	}
	if (v.mLineType != -1) c += " linetype " + std::to_string(v.mLineType);
	if (v.mLineWidth != -1) c += " linewidth " + std::to_string(v.mLineWidth);
	if (!v.mColor.empty()) c += " linecolor '" + v.mColor + "'";
	else if (v.mVariableColor) c += " linecolor palette";
	return std::move(c);
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

	if (!f.mFillColor.empty()) c += " linecolor '" + f.mFillColor + "'";
	else if (f.mVariableColor) c += " linecolor palette";
	{
		std::string fs;
		if (f.mTransparent != -1 && f.mPattern == -1) fs += Format(" transparent solid %lf", f.mTransparent);
		else if (f.mTransparent != -1 && f.mPattern != -1) fs += Format(" transparent pattern %d", f.mPattern);
		else if (f.mPattern != -1) fs += Format(" pattern %d", f.mPattern);
		if (!fs.empty()) c += " fillstyle" + fs;
	}
	{
		std::string bd;
		if (f.mBorderType != -1) bd += Format(" %d", f.mBorderType);
		if (!f.mBorderColor.empty()) bd += " linecolor '" + f.mBorderColor + "'";
		if (!bd.empty()) c += " border" + bd;
	}
	return std::move(c);
}
template <class GraphParam>
std::string ColormapPlotCommand(const GraphParam& p)
{

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
	void SetLog##AXIS();\
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
inline void GPM##AXIS##Axis<GPM>::SetLog##AXIS() { this->SetLog(axis); }\
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
using GPM2DAxis = GPMXAxis<GPMX2Axis<GPMYAxis<GPMY2Axis<GPMCBAxis<GPM>>>>>;

template <class GPM>
using GPM3DAxis = GPMZAxis<GPM2DAxis<GPM>>;

}

enum class Style { none, lines, points, linespoints, dots, impulses, boxes, steps, fsteps, histeps, };
enum class Smooth { none, unique, frequency, cumulative, cnormal, kdensity, csplines, acsplines, bezier, sbezier, };
enum class ArrowHead { head = 0, heads = 1, noheads = 2, filled = 0 << 2, empty = 1 << 2, nofilled = 2 << 2, };

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
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(filltransparent, double, FillOption)
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
		: mLineType(-1), mLineWidth(-1), mPointType(-1), mPointSize(-1),
		mStyle(Style::none), mSmooth(Smooth::none)
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
		mLineType = GetKeywordArg(plot::linetype, ops..., -1);
		mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
		mColor = GetKeywordArg(plot::color, ops..., "");
	}

	//LineOption
	int mLineType;//-1ならデフォルト
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
		: mLineType(-1), mLineWidth(-1), mArrowHead(0)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		mLineType = GetKeywordArg(plot::linetype, ops..., -1);
		mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
		mColor = GetKeywordArg(plot::color, ops..., "");
		if (KeywordExists(plot::variable_color, ops...)) mVariableColor = GetKeywordArg(plot::variable_color, ops...);
	}

	//LineOption
	int mLineType;//-1ならデフォルト
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
		: mClosed(false), mAbove(false), mBelow(false), mPattern(-1), mTransparent(-1), mBorderType(-1)
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
		mTransparent = GetKeywordArg(plot::filltransparent, ops..., -1);
		mBorderColor = GetKeywordArg(plot::bordercolor, ops..., "");
		mBorderType = GetKeywordArg(plot::bordertype, ops..., -1);
	}

	//FillOption
	std::string mFillColor;
	plot::ArrayData mVariableColor;
	std::string mBaseline;
	int mPattern;
	double mTransparent;
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
	GPMPlotBuffer2D(GPMPlotBuffer2D&& p);
	virtual ~GPMPlotBuffer2D();

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& y2,
									 Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	GPMPlotBuffer2D PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
								const std::vector<double>& xlen, const std::vector<double>& ylen,
								Options ...ops);
protected:

	GPMPlotBuffer2D Plot(GraphParam& i);

	static std::string PlotCommand(const GraphParam& i);
	static std::string InitCommand();

	std::vector<GraphParam> mParam;
	GPMCanvas* mCanvas;
};

template <class GraphParam, template <class> class Buffer>
class GPMCanvas2D : public GPM2DAxis<GPMCanvas>
{
public:

	using _Buffer = Buffer<GraphParam>;
	using detail::GPM2DAxis<GPMCanvas>::GPM2DAxis;

	friend class GPMMultiPlotter;

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	_Buffer PlotVectors(const std::vector<double>& xbegin, const std::vector<double>& ybegin,
						const std::vector<double>& xlen, const std::vector<double>& ylen,
						Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& y2,
							 Options ...ops);

};

template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam>::GPMPlotBuffer2D(GPMCanvas* g)
	: mCanvas(g) {}
template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam>::GPMPlotBuffer2D(GPMPlotBuffer2D&& p)
	: mCanvas(p.mCanvas), mParam(std::move(p.mParam))
{
	p.mCanvas = nullptr;
}
template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam>::~GPMPlotBuffer2D()
{
	//mPipeがnullptrでないときはこのPlotterが最終処理を担当する。
	if (mCanvas != nullptr)
	{
		std::string c = "plot";
		for (auto& i : mParam)
		{
			c += PlotCommand(i) + ", ";
		}
		c.erase(c.end() - 2, c.end());
		mCanvas->Command(c);
		mCanvas->Command(InitCommand());
	}
}
template <class GraphParam>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::Plot(GraphParam& i)
{
	if (i.mType == GraphParam::DATA)
	{
		i.mGraph = mCanvas->GetOutput() + ".tmp" + std::to_string(mParam.size()) + ".txt";
		auto GET_ARRAY = [](plot::ArrayData& X, const std::string& x,
							std::vector<std::vector<double>::const_iterator>& it, std::vector<std::string>& column, size_t& size)
		{
			switch (X.GetType())
			{
			case 0:
				if (size == 0) size = X.GetVector().size();
				else if (size != X.GetVector().size()) throw InvalidArg("The number of " + x + " does not match with the others.");
				it.emplace_back(X.GetVector().begin());
				column.emplace_back(std::to_string(it.size()));
				break;
			case 1:
				column.emplace_back(X.GetColumn());
				break;
			case 2:
				column.emplace_back("($1-$1+" + std::to_string(X.GetValue()) + ")");
				break;
			}
		};

		std::vector<std::vector<double>::const_iterator> it;
		std::vector<std::string> column;
		size_t size = 0;

		//ファイルを作成する。
		if (i.IsPoint())
		{
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");

			GET_ARRAY(p.mX, "x", it, column, size);
			GET_ARRAY(p.mY, "y", it, column, size);

			if (p.mXErrorbar) GET_ARRAY(p.mXErrorbar, "xerrorbar", it, column, size);
			if (p.mYErrorbar) GET_ARRAY(p.mYErrorbar, "yerrorbar", it, column, size);
			if (p.mVariableColor) GET_ARRAY(p.mVariableColor, "variable_color", it, column, size);
			if (p.mVariableSize) GET_ARRAY(p.mVariableSize, "variable_size", it, column, size);
		}
		else if (i.IsVector())
		{
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(v.mX, "x", it, column, size);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(v.mY, "y", it, column, size);
			if (!v.mXLen) throw InvalidArg("xlen coordinate list is not given.");
			GET_ARRAY(v.mXLen, "xlen", it, column, size);
			if (!v.mYLen) throw InvalidArg("ylen coordinate list is not given.");
			GET_ARRAY(v.mYLen, "ylen", it, column, size);

			if (v.mVariableColor) GET_ARRAY(v.mVariableColor, "variable_color", it, column, size);
		}
		else if (i.IsFilledCurve())
		{
			auto& f = i.GetFilledCurveParam();
			if (!f.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(f.mX, "x", it, column, size);
			if (!f.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(f.mY, "y", it, column, size);

			if (f.mY2) GET_ARRAY(f.mY2, "y2", it, column, size);
			if (f.mVariableColor) GET_ARRAY(f.mVariableColor, "variable_fillcolor", it, column, size);
		}
		MakeFile(it, size, i.mGraph);
		i.mColumn = std::move(column);
	}
	mParam.emplace_back(std::move(i));
	return std::move(*this);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	return PlotPoints(x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(const std::string& equation, Options ...ops)
{
	return PlotPoints(equation, plot::style = Style::lines, ops...);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	GraphParam i;
	i.AssignVector();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);

	auto& v = i.GetVectorParam();
	v.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
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
PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& y2,
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
inline std::string GPMPlotBuffer2D<GraphParam>::PlotCommand(const GraphParam& p)
{
	//filename or equation
	std::string c;
	if (p.mType == GraphParam::EQUATION)
	{
		//equation
		c += " " + p.mGraph;
	}
	else if (p.mType == GraphParam::DATA || p.mType == GraphParam::FILE)
	{
		//filename
		c += " '" + p.mGraph + "'";

		//using
		c += " using ";
		for (int i = 0; i < p.mColumn.size(); ++i)
			c += p.mColumn[i] + ":";
		c.pop_back();
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

	return std::move(c);
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
	return std::move(c);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(x, y, ops...);
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(x, y, ops...);
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	_Buffer r(this);
	return r.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& y2,
				 Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(x, y, y2, ops...);
}

}

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

}


namespace detail
{

struct GPMPointParamCM : public GPMPointParam
{
	plot::ArrayData mZ;
};
struct GPMVectorParamCM : public GPMVectorParam
{
	plot::ArrayData mZ;
	plot::ArrayData mZLen;
};
struct GPMFilledCurveParamCM : public GPMFilledCurveParam
{
	plot::ArrayData mZ;
};
struct GPMColormapParam
{
	GPMColormapParam()
		: mXRange(DBL_MAX, -DBL_MAX), mYRange(DBL_MAX, -DBL_MAX)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{

	}
	//ColormapOption
	//int mThinout;//要素ごとにプロットするときに、ビンあたりの要素がこの値を超えている場合、乱数でこの数程度になるように間引く。
	//int mEntry;//COLLECTIONのとき、値が-1でなければタイトルにエントリー数を追加する。
	//size_t mNormalize;//-1なら規格化しない。
	plot::ArrayData mXCoord;
	plot::ArrayData mYCoord;
	std::pair<double, double> mXRange;
	std::pair<double, double> mYRange;
	plot::MatrixData mZMap;
};

struct GPMGraphParamCM
	: public GPMGraphParamBase<GPMPointParamCM, GPMVectorParamCM, GPMFilledCurveParamCM, GPMColormapParam>
{
	void AssignPoint() { Emplace<GPMPointParamCM>(); }
	void AssignVector() { Emplace<GPMVectorParamCM>(); }
	void AssignFilledCurve() { Emplace<GPMFilledCurveParamCM>(); }
	void AssignColormap() { Emplace<GPMColormapParam>(); }

	bool IsPoint() const { return Is<GPMPointParamCM>(); }
	bool IsVector() const { return Is<GPMVectorParamCM>(); }
	bool IsFilledCurve() const { return Is<GPMFilledCurveParamCM>(); }
	bool IsColormap() const { return Is<GPMColormapParam>(); }

	GPMPointParamCM& GetPointParam() { return Get<GPMPointParamCM>(); }
	const GPMPointParamCM& GetPointParam() const { return Get<GPMPointParamCM>(); }
	GPMVectorParamCM& GetVectorParam() { return Get<GPMVectorParamCM>(); }
	const GPMVectorParamCM& GetVectorParam() const { return Get<GPMVectorParamCM>(); }
	GPMFilledCurveParamCM& GetFilledCurveParam() { return Get<GPMFilledCurveParamCM>(); }
	const GPMFilledCurveParamCM& GetFilledCurveParam() const { return Get<GPMFilledCurveParamCM>(); }
	GPMColormapParam& GetColormapParam() { return Get<GPMColormapParam>(); }
	const GPMColormapParam& GetColormapParam() const { return Get<GPMColormapParam>(); }
};

template <class GraphParam>
struct GPMPlotBufferCM
{
	GPMPlotBufferCM(GPMCanvas* g);
	GPMPlotBufferCM(const GPMPlotBufferCM&) = delete;
	GPMPlotBufferCM(GPMPlotBufferCM&& p);
	virtual ~GPMPlotBufferCM();

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(const std::string& equation, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
								  const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
								  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
								  const std::vector<double>& xlen, const std::vector<double>& ylen,
								  Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
								   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
								   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const std::string& equation, Options ...ops);

protected:

	GPMPlotBufferCM Plot(GraphParam& i);

	static std::string PlotCommand(const GraphParam& i);
	static std::string InitCommand();

	std::vector<GraphParam> mParam;
	GPMCanvas* mCanvas;
};

template <class GraphParam, template <class> class Buffer>
class GPMCanvasCM : public detail::GPM2DAxis<GPMCanvas>
{
public:

	using _Buffer = Buffer<GraphParam>;

	GPMCanvasCM(const std::string& output);
	GPMCanvasCM();

	friend class GPMMultiPlotter;

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
						  const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
						  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
						  const std::vector<double>& xlen, const std::vector<double>& ylen,
						  Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
						   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
						   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const std::string& equation, Options ...ops);
};

template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>::GPMPlotBufferCM(GPMCanvas* g)
	: mCanvas(g) {}
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>::GPMPlotBufferCM(GPMPlotBufferCM&& p)
	: mCanvas(p.mCanvas), mParam(std::move(p.mParam))
{
	p.mCanvas = nullptr;
}
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam>::~GPMPlotBufferCM()
{
	//mPipeがnullptrでないときはこのPlotterが最終処理を担当する。
	if (mCanvas != nullptr)
	{
		std::string c = "splot";
		for (auto& i : mParam)
		{
			c += PlotCommand(i) + ", ";
		}
		c.erase(c.end() - 2, c.end());
		mCanvas->Command(c);
		mCanvas->Command(InitCommand());
	}
}
template <class GraphParam>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::Plot(GraphParam& i)
{
	i.mGraph = mCanvas->GetOutput() + ".tmp" + std::to_string(mParam.size()) + ".txt";
	auto GET_ARRAY = [](plot::ArrayData& X, const std::string& x,
						std::vector<std::vector<double>::const_iterator>& it, std::vector<std::string>& column, size_t& size)
	{
		switch (X.GetType())
		{
		case 0:
			if (size == 0) size = X.GetVector().size();
			else if (size != X.GetVector().size()) throw InvalidArg("The number of " + x + " does not match with the others.");
			it.emplace_back(X.GetVector().begin());
			column.emplace_back(std::to_string(it.size()));
			break;
		case 1:
			column.emplace_back(X.GetColumn());
			break;
		case 2:
			column.emplace_back("($1-$1+" + std::to_string(X.GetValue()) + ")");
			break;
		}
	};

	std::vector<std::string> column;

	//ファイルを作成する。
	if (i.IsColormap())
	{
		size_t xsize = 0;
		size_t ysize = 0;
		auto& m = i.GetColormapParam();
		if (!m.mZMap) throw InvalidArg("z map is not given");
		if (m.mZMap.GetType() == 0)
		{
			//mapがMatrixであるとき、x、yの座標値も配列かrangeで与えられていなければならない。
			if (!((m.mXCoord && m.mXCoord.GetType() == 0) ||
				  m.mXRange != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");
			if (!((m.mYCoord && m.mYCoord.GetType() == 0) ||
				  m.mYRange != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");

			xsize = m.mZMap.GetMatrix().GetSize(0);
			ysize = m.mZMap.GetMatrix().GetSize(1);

			column = { "1", "2", "3" };
			if (m.mXCoord)
			{
				const auto& x = m.mXCoord.GetVector();
				auto getx = [x](int32_t i) { return x[i]; };
				if (m.mYCoord)
				{
					const auto& y = m.mYCoord.GetVector();
					auto gety = [y](int32_t i) { return y[i]; };
					MakeFile(m.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
				else
				{
					auto yrange = m.mYRange;
					double ywidth = yrange.second - yrange.first;
					auto gety = [yrange, ysize, ywidth](int32_t iy)
					{
						return ywidth * iy / ysize + yrange.first;
					};
					MakeFile(m.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
			}
			else
			{
				auto xrange = m.mXRange;
				double xwidth = xrange.second - xrange.first;
				auto getx = [xrange, xsize, xwidth](int32_t ix)
				{
					return xwidth * ix / xsize + xrange.first;
				};
				if (m.mYCoord)
				{
					const auto& y = m.mYCoord.GetVector();
					auto gety = [y](int32_t i) { return y[i]; };
					MakeFile(m.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
				else
				{
					auto yrange = m.mYRange;
					double ywidth = yrange.second - yrange.first;
					auto gety = [yrange, ysize, ywidth](int32_t iy)
					{
						return ywidth * iy / ysize + yrange.first;
					};
					MakeFile(m.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
			}
		}
		else
		{
			//既に存在するファイルから描画する場合。
			//x、yも同じファイルから取得しなければならない。原理的に。
			if (!m.mXCoord || m.mXCoord.GetType() != 1) throw InvalidArg("");
			if (!m.mYCoord || m.mYCoord.GetType() != 1) throw InvalidArg("");

			column.resize(3);
			column[0] = m.mXCoord.GetColumn();
			column[1] = m.mYCoord.GetColumn();

			if (m.mZMap.GetType() == 1)
				column[2] = m.mZMap.GetColumn();
			//matrixの固定値とか全く意味がわからんが、
			//仕様として一応許しておく。
			//背景色にしたい人とかがいるかも知らん。
			//......Gnuplotって背景色設定できるよな？
			if (m.mZMap.GetType() == 2)
				column[2] = "($1-$1+" + std::to_string(m.mZMap.GetValue()) + ")";
		}
	}
	else if (i.IsPoint())
	{
		std::vector<std::vector<double>::const_iterator> it;
		size_t size = 0;
		auto& p = i.GetPointParam();
		if (!p.mX) throw InvalidArg("x coordinate list is not given.");
		GET_ARRAY(p.mX, "x", it, column, size);
		if (!p.mY) throw InvalidArg("y coordinate list is not given.");
		GET_ARRAY(p.mY, "y", it, column, size);
		if (!p.mZ) throw InvalidArg("z coordinate list is not given.");
		GET_ARRAY(p.mZ, "z", it, column, size);

		if (p.mXErrorbar)
		{
			GET_ARRAY(p.mXErrorbar, "xerrorbar", it, column, size);
		}
		if (p.mYErrorbar)
		{
			GET_ARRAY(p.mYErrorbar, "yerrorbar", it, column, size);
		}
		if (p.mVariableColor)
		{
			GET_ARRAY(p.mVariableColor, "variable_color", it, column, size);
		}
		if (p.mVariableSize)
		{
			GET_ARRAY(p.mVariableSize, "variable_size", it, column, size);
		}
		MakeFile(it, size, i.mGraph);
	}
	else if (i.IsVector())
	{
		std::vector<std::vector<double>::const_iterator> it;
		size_t size = 0;
		auto& v = i.GetVectorParam();
		if (!v.mX) throw InvalidArg("x coordinate list is not given.");
		GET_ARRAY(v.mX, "x", it, column, size);
		if (!v.mY) throw InvalidArg("y coordinate list is not given.");
		GET_ARRAY(v.mY, "y", it, column, size);
		if (!v.mZ) throw InvalidArg("z coordinate list is not given.");
		GET_ARRAY(v.mZ, "z", it, column, size);
		if (!v.mXLen) throw InvalidArg("xlen coordinate list is not given.");
		GET_ARRAY(v.mXLen, "xlen", it, column, size);
		if (!v.mYLen) throw InvalidArg("ylen coordinate list is not given.");
		GET_ARRAY(v.mYLen, "ylen", it, column, size);
		if (!v.mZLen) throw InvalidArg("zlen coordinate list is not given.");
		GET_ARRAY(v.mZLen, "zlen", it, column, size);

		if (v.mVariableColor)
		{
			GET_ARRAY(v.mVariableColor, "variable_color", it, column, size);
		}
		MakeFile(it, size, i.mGraph);
	}
	i.mColumn = std::move(column);
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
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

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
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

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
PlotPoints(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::EQUATION;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");
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
	return PlotPoints(x, y, z, Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	return PlotPoints(x, y, Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(const std::string& equation, Options ...ops)
{
	return PlotPoints(equation, Style::lines, ops...);
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
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//vector
	auto& v = i.GetVectorParam();
	v.mX = xfrom;
	v.mY = yfrom;
	v.mZ = zfrom;
	v.mXLenVector = xlen;
	v.mYLenVector = ylen;
	v.mZLenVector = zlen;
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
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

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
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

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
PlotColormap(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.AssignColormap();
	i.mGraph = equation;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

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
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	//i.mStyle = GetKeywordArg(plot::style, ops..., plot::LINES);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//map
	auto& m = i.GetColormapParam();
	m.mZMap = map;
	m.mXRange = x;
	m.mYRange = y;
	m.SetOptions(ops...);
	return Plot(i);
}

template <class GraphParam>
inline std::string GPMPlotBufferCM<GraphParam>::PlotCommand(const GraphParam& p)
{
	//filename or equation
	std::string c;
	if (p.mType == GraphParam::EQUATION)
	{
		//equation
		c += " " + p.mGraph;
	}
	else if (p.mType == GraphParam::DATA || p.mType == GraphParam::FILE)
	{
		//filename
		c += "'" + p.mGraph + "'";

		//using
		c += " using ";
		for (int i = 0; i < p.mColumn.size(); ++i)
			c += p.mColumn[i] + ":";
		c.pop_back();
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

	return std::move(c);
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
	c += "unset pm3d";
	return std::move(c);
}

template <class GraphParam, template <class> class Buffer>
inline GPMCanvasCM<GraphParam, Buffer>::GPMCanvasCM(const std::string& output)
	: detail::GPM2DAxis<GPMCanvas>(output)
{
	if (mPipe)
	{
		Command("set pm3d corners2color c1");
		Command("set view map");
	}
}
template <class GraphParam, template <class> class Buffer>
inline GPMCanvasCM<GraphParam, Buffer>::GPMCanvasCM()
{
	if (mPipe)
	{
		Command("set pm3d corners2color c1");
		Command("set view map");
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
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(x, y, ops...);
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
PlotLines(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(x, y, ops...);
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
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
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
PlotColormap(const std::string& equation, Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(equation, ops...);
}

}

using GPMCanvas2D = detail::GPMCanvas2D<detail::GPMGraphParam2D, detail::GPMPlotBuffer2D>;
using GPMCanvasCM = detail::GPMCanvasCM<detail::GPMGraphParamCM, detail::GPMPlotBufferCM>;

class GPMMultiPlotter
{
public:
	GPMMultiPlotter(const std::string& outputname, int row, int column);
	~GPMMultiPlotter();

	void Begin(const std::string& output, int row, int column);
	void End();

	void Command(const std::string& c);
private:
};

inline GPMMultiPlotter::GPMMultiPlotter(const std::string& output, int row, int column)
{
	Begin(output, row, column);
}
inline GPMMultiPlotter::~GPMMultiPlotter()
{
	End();
}
inline void GPMMultiPlotter::Begin(const std::string& output, int row, int column)
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
				Command("set terminal pngcairo enhanced size "
						+ std::to_string(column * 640) + ", " + std::to_string(row * 480) + "\nset output '" + repout + "'");
			else if (extension == ".eps")
				Command("set terminal epscairo enhanced size "
						+ std::to_string(column * 6) + "in, " + std::to_string(row * 4) + "in\nset output '" + repout + "'");
			else if (extension == ".pdf")
				Command("set terminal pdfcairo enhanced size "
						+ std::to_string(column * 6) + "in, " + std::to_string(row * 4) + "in\nset output '" + repout + "'");
		}
		else if (output == "wxt");
		else std::cout << "WARNING : " << output << " is not a terminal or has no valid extension. Default terminal is selected." << std::endl;

		Command("set multiplot layout " + std::to_string(row) + ", " + std::to_string(column));
	}
}
inline void GPMMultiPlotter::End()
{
	if (GPMCanvas::Paths<>::msGlobalPipe != nullptr)
	{
		Command("unset multiplot");
		_pclose(GPMCanvas::Paths<>::msGlobalPipe);
		GPMCanvas::Paths<>::msGlobalPipe = nullptr;
	}
}
inline void GPMMultiPlotter::Command(const std::string& str)
{
	fprintf(GPMCanvas::Paths<>::msGlobalPipe, (str + "\n").c_str());
	fflush(GPMCanvas::Paths<>::msGlobalPipe);
}

}

}

#endif