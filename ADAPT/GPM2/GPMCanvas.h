﻿//
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

namespace adapt
{

namespace gpm2
{

namespace plot
{

struct ArrayData
{
	ArrayData() : mVector(nullptr), mType(-1) {}
	ArrayData(const std::vector<double>& vector) : mVector(&vector), mType(0) {}
	ArrayData(const std::string& column) : mColumn(&column), mType(1) {}
	ArrayData(double value) : mValue(value), mType(2) {}
	ArrayData(const ArrayData& a) : mVector(a.mVector), mType(a.mType) {}

	bool IsEmpty() const { return (!mVector) && (mType == -1); }
	int GetType() const { return mType; }
	const std::vector<double>& GetVector() const { return *mVector; }
	const std::string& GetColumn() const { return *mColumn; }
	double GetValue() const { return mValue; }

	operator bool() const { return !IsEmpty(); }

private:

	union
	{
		const std::vector<double>* mVector;
		const std::string* mColumn;
		double mValue;
	};
	int mType;//0:vector, 1:column, 2:constant value
};

struct MatrixData
{
	MatrixData() : mMatrix(nullptr), mType(-1) {}
	MatrixData(const Matrix<double>& vector) : mMatrix(&vector), mType(0) {}
	MatrixData(const std::string& column) : mColumn(&column), mType(1) {}
	MatrixData(double value) : mValue(value), mType(2) {}

	bool IsEmpty() const { return (!mMatrix) && (mType == -1); }
	int GetType() const { return mType; }
	const Matrix<double>& GetMatrix() const { return *mMatrix; }
	const std::string& GetColumn() const { return *mColumn; }
	double GetValue() const { return mValue; }

	operator bool() const { return !IsEmpty(); }

private:

	union
	{
		const Matrix<double>* mMatrix;
		const std::string* mColumn;
		double mValue;
	};
	int mType;//0:vector, 1:column, 2:constant value
};

}

class GPMMultiPlotter;

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

template <class GraphParam>
std::string PointPlotCommand(const GraphParam& p)
{
	std::string c;
	if (p.mXErrorbarPoint || p.mYErrorbarPoint)
	{
		if (p.mStyle == Style::LINES)
		{
			if (p.mXErrorbarPoint && p.mYErrorbarPoint)
				c += " xyerrorlines";
			else if (p.mXErrorbarPoint)
				c += " xerrorlines";
			else
				c += " yerrorlines";
			if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
			if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
			if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
			else if (p.mVariableColorPoint) c += " linecolor palette";
		}
		else if (p.mStyle == Style::BOXES)
		{
			//c += " boxes";
			if (p.mXErrorbarPoint && p.mYErrorbarPoint)
				c += " boxxyerrorbars";
			else if (p.mXErrorbarPoint)
				std::cerr << "WARNING : Box style is incompatible with xerrorbar option. It is to be ignored." << std::endl;
			else
				c += " boxerrorbars";
			if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
			if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
			if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
			else if (p.mVariableColorPoint) c += " linecolor palette";
		}
		else
		{
			if (p.mStyle != Style::POINTS)
				std::cerr << "WARNING : Only \"lines\", \"boxes\" or \"points\" styles are allowed with errobars. Style option is to be ignored." << std::endl;

			//c += " points";
			if (p.mXErrorbarPoint && p.mYErrorbarPoint)
				c += " xyerrorbars";
			else if (p.mXErrorbarPoint)
				c += " xerrorbars";
			else if (p.mYErrorbarPoint)
				c += " yerrorbars";
			if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
			if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
			if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";//pointのときも何故かlinecolorらしい。
			else if (p.mVariableColorPoint) c += " palette";//しかしpalette指定の場合はlinecolorがいらない。謎。
		}
	}
	//ベクトル、エラーバー指定がない場合。
	else if (p.mStyle == Style::LINES ||
			 p.mStyle == Style::INPULSES ||
			 p.mStyle == Style::STEPS ||
			 p.mStyle == Style::FSTEPS ||
			 p.mStyle == Style::HISTEPS ||
			 p.mStyle == Style::BOXES)
	{
		switch (p.mStyle)
		{
		case Style::LINES: c += " lines"; break;
		case Style::INPULSES: c += " impulses"; break;
		case Style::STEPS: c += " steps"; break;
		case Style::FSTEPS: c += " fsteps"; break;
		case Style::HISTEPS: c += " histeps"; break;
		case Style::BOXES: c += " boxes"; break;
		}
		if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColorPoint) c += " linecolor palette";
	}
	else if (p.mStyle == Style::POINTS)
	{
		c += " points";
		if (p.mLineType != -1)
			std::cerr << "WARNING : \"points\" style is incompatible with linetype option. It is to be ignored." << std::endl;
		if (p.mLineWidth != -1)
			std::cerr << "WARNING : \"points\" style is incompatible with linewidth option. It is to be ignored." << std::endl;
		if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
		if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";//pointのときも何故かlinecolorらしい。
		else if (p.mVariableColorPoint) c += " palette";//しかしpalette指定の場合はlinecolorがいらない。謎。
	}
	else if (p.mStyle == Style::LINESPOINTS)
	{
		c += " linespoints";
		if (p.mLineType != -1) c += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) c += " linewidth " + std::to_string(p.mLineWidth);
		if (p.mPointType != -1) c += " pointtype " + std::to_string(p.mPointType);
		if (p.mPointSize != -1) c += " pointsize " + std::to_string(p.mPointSize);
		if (!p.mColor.empty()) c += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColorPoint) c += " linecolor palette";
	}
	else if (p.mStyle == Style::DOTS)
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
		else if (p.mVariableColorPoint) c += "palette ";
	}
	return std::move(c);
}
template<class GraphParam>
std::string VectorPlotCommand(const GraphParam& p)
{
	std::string c;
	c += " vector ";
	if (p.mArrowHead != -1)
	{
		if ((p.mArrowHead & 0b11) == 0) c += " head";
		else if ((p.mArrowHead & 0b11) == 1) c += " heads";
		else if ((p.mArrowHead & 0b11) == 2) c += " noheads";
		if ((p.mArrowHead & 0b1100) == 4) c += " filled";
		else if ((p.mArrowHead & 0b1100) == 5) c += " empty";
		else if ((p.mArrowHead & 0b1100) == 6) c += " nofilled";
	}
	if (p.mArrowWidth != -1) c += " linewidth " + std::to_string(p.mArrowWidth);
	if (!p.mArrowColor.empty()) c += " linecolor '" + p.mArrowColor + "'";
	else if (p.mVariableColorVector) c += " linecolor palette";
	return std::move(c);
}
template <class GraphParam>
std::string ColormapPlotCommand(const GraphParam& p)
{

}


class GPMCanvas
{
public:

	friend class GPMMultiPlotter;

	GPMCanvas(const std::string& output);
	GPMCanvas();
	GPMCanvas(const GPMCanvas&) = delete;
	GPMCanvas(GPMCanvas&&) noexcept;
	GPMCanvas& operator=(const GPMCanvas&) = delete;
	GPMCanvas& operator=(GPMCanvas&&);
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
inline GPMCanvas::GPMCanvas(GPMCanvas&& gpm) noexcept
{
	*this = std::move(gpm);
}
inline GPMCanvas& GPMCanvas::operator=(GPMCanvas&& gpm)
{
	mOutput = std::move(gpm.mOutput);
	mPipe = gpm.mPipe; gpm.mPipe = nullptr;
	return *this;
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
	Command(Format("set %srange [:max]", axis, max));
}

void GPMCanvas::SetLog(const std::string& axis, double scale)
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

inline void Command(FILE* pipe, const std::string& str)
{
	fprintf(pipe, (str + "\n").c_str());
	fflush(pipe);
}

inline void GPMCanvas::Command(const std::string& str)
{
	gpm2::detail::Command(mPipe, str);
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
using GPM3DAxis = GPMXAxis<GPMX2Axis<GPMYAxis<GPMY2Axis<GPMZAxis<GPMCBAxis<GPM>>>>>>;

}

enum class Style { DEFAULT, LINES, POINTS, LINESPOINTS, DOTS, INPULSES, BOXES, STEPS, FSTEPS, HISTEPS, };
enum class ArrowStyle { HEAD = 0, HEADS = 1, NOHEADS = 2, FILLED = 0 << 2, EMPTY = 1 << 2, NOFILLED = 2 << 2, };

namespace plot
{

struct BaseOption {};
struct PointOption : public BaseOption {};
struct VectorOption : public BaseOption {};
struct PointVectorOption : public PointOption, public VectorOption {};

CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(title, const std::string&, BaseOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(style, Style, BaseOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(axis, const std::string&, BaseOption)//y2軸を使いたい場合などに、"x1y2"のように指定する。

CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(pointtype, int, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(pointsize, double, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(linetype, int, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(linewidth, double, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(color, const std::string&, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(smooth, const std::string&, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(x, ArrayData, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(y, ArrayData, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(xerrorbar, ArrayData, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(yerrorbar, ArrayData, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(variable_size, ArrayData, PointOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(variable_color, ArrayData, PointOption)

CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(xfrom, ArrayData, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(xlen, ArrayData, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(yfrom, ArrayData, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(ylen, ArrayData, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(arrowcolor, const std::string&, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(arrowwidth, double, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(variable_arrowcolor, ArrayData, VectorOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(arrowhead, int, VectorOption)

}

namespace detail
{

namespace plot = gpm2::plot;

struct GPMGraphParam2D
{
	GPMGraphParam2D()
		: mType(EQUATION), mAxis(),
		mStyle(Style::DEFAULT), mPointType(-1), mPointSize(-1), mLineType(-1), mLineWidth(-1), mColor(),
		mArrowWidth(-1), mArrowColor(),
		mArrowHead(0)
	{}
	virtual ~GPMGraphParam2D() = default;

	bool IsPoint() const
	{
		return mPointType != -1 ||
			mPointSize != -1 ||
			mLineType != -1 ||
			mLineWidth != -1 ||
			mStyle != Style::DEFAULT ||
			!mColor.empty() ||
			!mXPoint.IsEmpty() ||
			!mYPoint.IsEmpty() ||
			!mXErrorbarPoint.IsEmpty() ||
			!mYErrorbarPoint.IsEmpty() ||
			!mVariableColorPoint.IsEmpty() ||
			!mVariableSizePoint.IsEmpty();
	}
	bool IsVector() const
	{
		return !mArrowColor.empty() ||
			mArrowWidth != -1 ||
			!mXFromVector.IsEmpty() ||
			!mYFromVector.IsEmpty() ||
			!mXLenVector.IsEmpty() ||
			!mYLenVector.IsEmpty() ||
			!mVariableColorVector.IsEmpty();
	}

	enum Type { EQUATION, FILE, DATA, };
	int mType;
	std::string mGraph;

	//Baseoption
	std::string mTitle;
	std::string mAxis;

	//PointOption
	Style mStyle;//デフォルトではPOINTS
	int mPointType;//-1ならデフォルト
	double mPointSize;//-1ならデフォルト、-2ならvariable
	int mLineType;//-1ならデフォルト
	double mLineWidth;//-1ならデフォルト、-2ならvariable
	std::string mColor;
	plot::ArrayData mXPoint;
	plot::ArrayData mYPoint;
	plot::ArrayData mXErrorbarPoint;
	plot::ArrayData mYErrorbarPoint;
	plot::ArrayData mVariableColorPoint;
	plot::ArrayData mVariableSizePoint;

	//VectorOption
	std::string mArrowColor;
	double mArrowWidth;
	plot::ArrayData mXFromVector;
	plot::ArrayData mYFromVector;
	plot::ArrayData mXLenVector;
	plot::ArrayData mYLenVector;
	plot::ArrayData mVariableColorVector;
	int mArrowHead;//-1ならデフォルト。

	std::vector<std::string> mColumn;
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
class GPMCanvas2D : public GPM2DAxis<detail::GPMCanvas>
{
public:

	using _Buffer = Buffer<GraphParam>;
	using detail::GPM2DAxis<detail::GPMCanvas>::GPM2DAxis;

	friend class GPMMultiPlotter;

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(const std::string& equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	_Buffer PlotVectors(const std::vector<double>& xbegin, const std::vector<double>& ybegin,
						  const std::vector<double>& xlen, const std::vector<double>& ylen,
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
	const bool ispoint = i.IsPoint();
	const bool isvector = i.IsVector();

	if ((int)ispoint + (int)isvector != 1) throw InvalidArg("");

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
		if (ispoint)
		{
			if (!i.mXPoint) throw InvalidArg("x coordinate list is not given.");
			if (!i.mYPoint) throw InvalidArg("y coordinate list is not given.");

			if (i.mXPoint.GetType() == 0)
			{

			}
			GET_ARRAY(i.mXPoint, "x", it, column, size);
			GET_ARRAY(i.mYPoint, "y", it, column, size);

			if (i.mXErrorbarPoint)
			{
				GET_ARRAY(i.mXErrorbarPoint, "xerrorbar", it, column, size);
			}
			if (i.mYErrorbarPoint)
			{
				GET_ARRAY(i.mYErrorbarPoint, "yerrorbar", it, column, size);
			}
			if (i.mVariableColorPoint)
			{
				GET_ARRAY(i.mVariableColorPoint, "variable_color", it, column, size);
			}
			if (i.mVariableSizePoint)
			{
				GET_ARRAY(i.mVariableSizePoint, "variable_size", it, column, size);
			}
		}
		else if (isvector)
		{
			if (!i.mXFromVector) throw InvalidArg("xfrom coordinate list is not given.");
			GET_ARRAY(i.mXFromVector, "xfrom", it, column, size);
			if (!i.mYFromVector) throw InvalidArg("yfrom coordinate list is not given.");
			GET_ARRAY(i.mYFromVector, "yfrom", it, column, size);
			if (!i.mXLenVector) throw InvalidArg("xlen coordinate list is not given.");
			GET_ARRAY(i.mXLenVector, "xlen", it, column, size);
			if (!i.mYLenVector) throw InvalidArg("ylen coordinate list is not given.");
			GET_ARRAY(i.mYLenVector, "ylen", it, column, size);

			if (i.mVariableColorVector)
			{
				GET_ARRAY(i.mVariableColorVector, "variable_color", it, column, size);
			}
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
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mStyle = GetKeywordArg(plot::style, ops..., Style::POINTS);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//point
	i.mXPoint = x;
	i.mYPoint = y;
	if (KeywordExists(plot::xerrorbar, ops...)) i.mXErrorbarPoint = GetKeywordArg(plot::xerrorbar, ops...);
	if (KeywordExists(plot::yerrorbar, ops...)) i.mYErrorbarPoint = GetKeywordArg(plot::yerrorbar, ops...);
	if (KeywordExists(plot::variable_color, ops...)) i.mVariableColorPoint = GetKeywordArg(plot::variable_color, ops...);
	if (KeywordExists(plot::variable_size, ops...)) i.mVariableSizePoint = GetKeywordArg(plot::variable_size, ops...);
	i.mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
	i.mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
	i.mLineType = GetKeywordArg(plot::linetype, ops..., -1);
	i.mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
	i.mColor = GetKeywordArg(plot::color, ops..., "");

	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotPoints(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.mGraph = equation;
	i.mType = GraphParam::EQUATION;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mStyle = GetKeywordArg(plot::style, ops..., Style::POINTS);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//point
	i.mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
	i.mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
	i.mLineType = GetKeywordArg(plot::linetype, ops..., -1);
	i.mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
	i.mColor = GetKeywordArg(plot::color, ops..., "");

	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	i.mXFromVector = xfrom;
	i.mYFromVector = yfrom;
	i.mXLenVector = xlen;
	i.mYLenVector = ylen;
	i.mArrowColor = GetKeywordArg(plot::arrowcolor, ops..., "");
	i.mArrowWidth = GetKeywordArg(plot::arrowwidth, ops..., -1);
	if (KeywordExists(plot::variable_arrowcolor, ops...)) i.mVariableColorVector = GetKeywordArg(plot::variable_arrowcolor, ops...);

	return Plot(i);
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
	const bool ispoint = p.IsPoint();
	const bool isvector = p.IsVector();

	c += " with";
	//ベクトル指定がある場合。
	if (isvector)
	{
		c += VectorPlotCommand(p);
	}
	//点指定がある場合。
	else
	{
		c += PointPlotCommand(p);
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
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen,
			Options ...ops)
{
	_Buffer r(this);
	return r.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
}

}

namespace plot
{

struct Point3DOption : public PointOption {};
struct Vector3DOption : public VectorOption {};
struct ColormapOption : public BaseOption {};

CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(z, ArrayData, Point3DOption)

CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(zfrom, ArrayData, Vector3DOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(zlen, ArrayData, Vector3DOption)

CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(map, CUF_TIE_ARGS(const Matrix<double, 2>&), ColormapOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(xcoord, const std::vector<double>&, ColormapOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(ycoord, const std::vector<double>&, ColormapOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(xmaprange, CUF_TIE_ARGS(std::pair<double, double>), ColormapOption)
CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(ymaprange, CUF_TIE_ARGS(std::pair<double, double>), ColormapOption)

}


namespace detail
{

struct GPMGraphParamCM : public GPMGraphParam2D
{
	GPMGraphParamCM()
		: mXRangeMap(DBL_MAX, -DBL_MAX), mYRangeMap(DBL_MAX, -DBL_MAX)
	{}
	virtual ~GPMGraphParamCM() = default;

	bool IsColorMap() const
	{
		return mXCoordMap ||
			mYCoordMap ||
			mXRangeMap != std::make_pair(DBL_MAX, -DBL_MAX) ||
			mYRangeMap != std::make_pair(DBL_MAX, -DBL_MAX) ||
			mZMap;
	}

	//PointOption
	plot::ArrayData mZPoint;

	//VectorOption
	plot::ArrayData mZFromVector;
	plot::ArrayData mZLenVector;

	//ColormapOption
	//int mThinout;//要素ごとにプロットするときに、ビンあたりの要素がこの値を超えている場合、乱数でこの数程度になるように間引く。
	//int mEntry;//COLLECTIONのとき、値が-1でなければタイトルにエントリー数を追加する。
	//size_t mNormalize;//-1なら規格化しない。
	plot::ArrayData mXCoordMap;
	plot::ArrayData mYCoordMap;
	std::pair<double, double> mXRangeMap;
	std::pair<double, double> mYRangeMap;
	plot::MatrixData mZMap;
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
class GPMCanvasCM : public detail::GPM2DAxis<detail::GPMCanvas>
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
	const bool ispoint = i.IsPoint();
	const bool isvector = i.IsVector();
	const bool ismap = i.IsColorMap();

	if ((int)ispoint + (int)isvector + (int)ismap != 1) throw InvalidArg("");

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
	if (ismap)
	{
		size_t xsize = 0;
		size_t ysize = 0;
		if (!i.mZMap) throw InvalidArg("z map is not given");
		if (i.mZMap.GetType() == 0)
		{
			//mapがMatrixであるとき、x、yの座標値も配列かrangeで与えられていなければならない。
			if (!((i.mXCoordMap && i.mXCoordMap.GetType() == 0) ||
				  i.mXRangeMap != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");
			if (!((i.mYCoordMap && i.mYCoordMap.GetType() == 0) ||
				  i.mYRangeMap != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");

			xsize = i.mZMap.GetMatrix().GetSize(0);
			ysize = i.mZMap.GetMatrix().GetSize(1);

			column = { "1", "2", "3" };
			if (i.mXCoordMap)
			{
				const auto& x = i.mXCoordMap.GetVector();
				auto getx = [x](int32_t i) { return x[i]; };
				if (i.mYCoordMap)
				{
					const auto& y = i.mYCoordMap.GetVector();
					auto gety = [y](int32_t i) { return y[i]; };
					MakeFile(i.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
				else
				{
					auto yrange = i.mYRangeMap;
					double ywidth = yrange.second - yrange.first;
					auto gety = [yrange, ysize, ywidth](int32_t iy)
					{
						return ywidth * iy / ysize + yrange.first;
					};
					MakeFile(i.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
			}
			else
			{
				auto xrange = i.mXRangeMap;
				double xwidth = xrange.second - xrange.first;
				auto getx = [xrange, xsize, xwidth](int32_t ix)
				{
					return xwidth * ix / xsize + xrange.first;
				};
				if (i.mYCoordMap)
				{
					const auto& y = i.mYCoordMap.GetVector();
					auto gety = [y](int32_t i) { return y[i]; };
					MakeFile(i.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
				else
				{
					auto yrange = i.mYRangeMap;
					double ywidth = yrange.second - yrange.first;
					auto gety = [yrange, ysize, ywidth](int32_t iy)
					{
						return ywidth * iy / ysize + yrange.first;
					};
					MakeFile(i.mZMap.GetMatrix(), getx, gety, i.mGraph);
				}
			}
		}
		else
		{
			//既に存在するファイルから描画する場合。
			//x、yも同じファイルから取得しなければならない。原理的に。
			if (!i.mXCoordMap || i.mXCoordMap.GetType() != 1) throw InvalidArg("");
			if (!i.mYCoordMap || i.mYCoordMap.GetType() != 1) throw InvalidArg("");

			column.resize(3);
			column[0] = i.mXCoordMap.GetColumn();
			column[1] = i.mYCoordMap.GetColumn();

			if (i.mZMap.GetType() == 1)
				column[2] = i.mZMap.GetColumn();
			//matrixの固定値とか全く意味がわからんが、
			//仕様として一応許しておく。
			//背景色にしたい人とかがいるかも知らん。
			//......Gnuplotって背景色設定できるよな？
			if (i.mZMap.GetType() == 2)
				column[2] = "($1-$1+" + std::to_string(i.mZMap.GetValue()) + ")";
		}
	}
	else if (ispoint)
	{
		std::vector<std::vector<double>::const_iterator> it;
		size_t size = 0;
		if (!i.mXPoint) throw InvalidArg("x coordinate list is not given.");
		GET_ARRAY(i.mXPoint, "x", it, column, size);
		if (!i.mYPoint) throw InvalidArg("y coordinate list is not given.");
		GET_ARRAY(i.mYPoint, "y", it, column, size);
		if (!i.mYPoint) throw InvalidArg("z coordinate list is not given.");
		GET_ARRAY(i.mYPoint, "z", it, column, size);

		if (i.mXErrorbarPoint)
		{
			GET_ARRAY(i.mXErrorbarPoint, "xerrorbar", it, column, size);
		}
		if (i.mYErrorbarPoint)
		{
			GET_ARRAY(i.mYErrorbarPoint, "yerrorbar", it, column, size);
		}
		if (i.mVariableColorPoint)
		{
			GET_ARRAY(i.mVariableColorPoint, "variable_color", it, column, size);
		}
		if (i.mVariableSizePoint)
		{
			GET_ARRAY(i.mVariableSizePoint, "variable_size", it, column, size);
		}
		MakeFile(it, size, i.mGraph);
	}
	else if (isvector)
	{
		std::vector<std::vector<double>::const_iterator> it;
		size_t size = 0;
		if (!i.mXFromVector) throw InvalidArg("xfrom coordinate list is not given.");
		GET_ARRAY(i.mXFromVector, "xfrom", it, column, size);
		if (!i.mYFromVector) throw InvalidArg("yfrom coordinate list is not given.");
		GET_ARRAY(i.mYFromVector, "yfrom", it, column, size);
		if (!i.mZFromVector) throw InvalidArg("zfrom coordinate list is not given.");
		GET_ARRAY(i.mZFromVector, "zfrom", it, column, size);
		if (!i.mXLenVector) throw InvalidArg("xlen coordinate list is not given.");
		GET_ARRAY(i.mXLenVector, "xlen", it, column, size);
		if (!i.mYLenVector) throw InvalidArg("ylen coordinate list is not given.");
		GET_ARRAY(i.mYLenVector, "ylen", it, column, size);
		if (!i.mZLenVector) throw InvalidArg("zlen coordinate list is not given.");
		GET_ARRAY(i.mZLenVector, "zlen", it, column, size);

		if (i.mVariableColorVector)
		{
			GET_ARRAY(i.mVariableColorVector, "variable_color", it, column, size);
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
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mStyle = GetKeywordArg(plot::style, ops..., Style::POINTS);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//point
	i.mXPoint = x;
	i.mYPoint = y;
	i.mZPoint = z;
	if (KeywordExists(plot::xerrorbar, ops...)) i.mXErrorbarPoint = GetKeywordArg(plot::xerrorbar, ops...);
	if (KeywordExists(plot::yerrorbar, ops...)) i.mYErrorbarPoint = GetKeywordArg(plot::yerrorbar, ops...);
	if (KeywordExists(plot::variable_color, ops...)) i.mVariableColorPoint = GetKeywordArg(plot::variable_color, ops...);
	if (KeywordExists(plot::variable_size, ops...)) i.mVariableSizePoint = GetKeywordArg(plot::variable_size, ops...);
	i.mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
	i.mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
	i.mLineType = GetKeywordArg(plot::linetype, ops..., -1);
	i.mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
	i.mColor = GetKeywordArg(plot::color, ops..., "");
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::vector<double>& x, const std::vector<double>& y, Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mStyle = GetKeywordArg(plot::style, ops..., Style::POINTS);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//point
	i.mXPoint = x;
	i.mYPoint = y;
	i.mZPoint = 0.;
	if (KeywordExists(plot::xerrorbar, ops...)) i.mXErrorbarPoint = GetKeywordArg(plot::xerrorbar, ops...);
	if (KeywordExists(plot::yerrorbar, ops...)) i.mYErrorbarPoint = GetKeywordArg(plot::yerrorbar, ops...);
	if (KeywordExists(plot::variable_color, ops...)) i.mVariableColorPoint = GetKeywordArg(plot::variable_color, ops...);
	if (KeywordExists(plot::variable_size, ops...)) i.mVariableSizePoint = GetKeywordArg(plot::variable_size, ops...);
	i.mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
	i.mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
	i.mLineType = GetKeywordArg(plot::linetype, ops..., -1);
	i.mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
	i.mColor = GetKeywordArg(plot::color, ops..., "");
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::EQUATION;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mStyle = GetKeywordArg(plot::style, ops..., Style::POINTS);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	i.mGraph = equation;
	//point
	i.mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
	i.mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
	i.mLineType = GetKeywordArg(plot::linetype, ops..., -1);
	i.mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
	i.mColor = GetKeywordArg(plot::color, ops..., "");
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotVectors(const std::vector<double>& xfrom, const std::vector<double>& yfrom, const std::vector<double>& zfrom,
			const std::vector<double>& xlen, const std::vector<double>& ylen, const std::vector<double>& zlen,
			Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//vector
	i.mXFromVector = xfrom;
	i.mYFromVector = yfrom;
	i.mZFromVector = zfrom;
	i.mXLenVector = xlen;
	i.mYLenVector = ylen;
	i.mZLenVector = zlen;
	i.mArrowColor = GetKeywordArg(plot::arrowcolor, ops..., "");
	i.mArrowWidth = GetKeywordArg(plot::arrowwidth, ops..., -1);
	if (KeywordExists(plot::variable_arrowcolor, ops...)) i.mVariableColorVector = GetKeywordArg(plot::variable_arrowcolor, ops...);

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
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//vector
	i.mXFromVector = xfrom;
	i.mYFromVector = yfrom;
	i.mZFromVector = 0.;
	i.mXLenVector = xlen;
	i.mYLenVector = ylen;
	i.mZLenVector = 0.;
	i.mArrowColor = GetKeywordArg(plot::arrowcolor, ops..., "");
	i.mArrowWidth = GetKeywordArg(plot::arrowwidth, ops..., -1);
	if (KeywordExists(plot::variable_arrowcolor, ops...)) i.mVariableColorVector = GetKeywordArg(plot::variable_arrowcolor, ops...);

	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const Matrix<double>& map, const std::vector<double>& x, const std::vector<double>& y,
			 Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//map
	i.mXCoordMap = x;
	i.mYCoordMap = y;
	i.mZMap = map;

	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const std::string& equation, Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//map
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const Matrix<double>& map, std::pair<double, double> x, std::pair<double, double> y,
			 Options ...ops)
{
	GraphParam i;
	i.mType = GraphParam::DATA;
	i.mTitle = GetKeywordArg(plot::title, ops..., "");
	//i.mStyle = GetKeywordArg(plot::style, ops..., plot::LINES);
	i.mAxis = GetKeywordArg(plot::axis, ops..., "");

	//map
	i.mZMap = map;
	i.mXRangeMap = x;
	i.mYRangeMap = y;

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

	const bool ispoint = p.IsPoint();
	const bool isvector = p.IsVector();
	const bool ismap = p.IsColorMap();

	c += " with";
	//カラーマップの場合。
	if (ismap)
	{
		c += " pm3d";
	}
	//ベクトルの場合。
	else if (isvector)
	{
		c += VectorPlotCommand(p);
	}
	//点の場合。
	else
	{
		c += PointPlotCommand(p);
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
	: detail::GPM2DAxis<detail::GPMCanvas>(output)
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
	if (detail::GPMCanvas::Paths<>::msGlobalPipe != nullptr)
	{
		std::cerr << "Gnuplot has already been open. " << detail::GPMCanvas::GetGnuplotPath() << std::endl;
		return;
	}
	if ((detail::GPMCanvas::Paths<>::msGlobalPipe = _popen(detail::GPMCanvas::GetGnuplotPath().c_str(), "w")) == nullptr)
	{
		std::cerr << "Gnuplot cannot open. " << detail::GPMCanvas::GetGnuplotPath() << std::endl;
	}
	else
	{
		detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "set bars small");
		detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "set palette defined ( 0 '#000090',1 '#000fff',2 '#0090ff',3 '#0fffee',4 '#90ff70',5 '#ffee00',6 '#ff7000',7 '#ee0000',8 '#7f0000')");

		if (output.size() > 4)
		{
			std::string extension = output.substr(output.size() - 4, 4);
			std::string repout = ReplaceStr(output, "\\", "/");
			if (extension == ".png")
				detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "set terminal pngcairo enhanced size "
								+ std::to_string(column * 640) + ", " + std::to_string(row * 480) + "\nset output '" + repout + "'");
			else if (extension == ".eps")
				detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "set terminal epscairo enhanced size "
								+ std::to_string(column * 6) + "in, " + std::to_string(row * 4) + "in\nset output '" + repout + "'");
			else if (extension == ".pdf")
				detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "set terminal pdfcairo enhanced size "
								+ std::to_string(column * 6) + "in, " + std::to_string(row * 4) + "in\nset output '" + repout + "'");
		}
		else if (output == "wxt");
		else std::cout << "WARNING : " << output << " is not a terminal or has no valid extension. Default terminal is selected." << std::endl;

		detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "set multiplot layout " + std::to_string(row) + ", " + std::to_string(column));
	}
}
inline void GPMMultiPlotter::End()
{
	if (detail::GPMCanvas::Paths<>::msGlobalPipe != nullptr)
	{
		detail::Command(detail::GPMCanvas::Paths<>::msGlobalPipe, "unset multiplot");
		fflush(detail::GPMCanvas::Paths<>::msGlobalPipe);
		_pclose(detail::GPMCanvas::Paths<>::msGlobalPipe);
		detail::GPMCanvas::Paths<>::msGlobalPipe = nullptr;
	}
}

}

}

#endif