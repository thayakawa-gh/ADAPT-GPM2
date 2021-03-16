#ifndef GPM2_GPMCANVAS_H
#define GPM2_GPMCANVAS_H

#include <ADAPT/CUF/Matrix.h>
#include <ADAPT/CUF/KeywordArgs.h>
#include <ADAPT/CUF/Function.h>
#include <ADAPT/GPM2/GPMArrayData.h>

namespace adapt
{

namespace gpm2
{

namespace detail
{

template <class OutputFunc>
inline void MakeDataObjectCommon(OutputFunc output_func, std::vector<ArrayData::Range>& its)
{
	while (true)
	{
		size_t eod_count = 0;
		for (auto& it : its)
		{
			it.Output(output_func);
			++it;
			eod_count += it.End() ? 1 : 0;//いずれか1つでも終端に達したら打ち切る。
		}
		output_func("\n", print::end<>());
		if (eod_count)
		{
			if (eod_count != its.size())
				std::cout <<
					"WARNING : The numbers of data points are inconsistent."
					" The trailing data points are truncated to fit the smallest data set." << std::endl;
			break;
		}
	}
}

template <class OutputFunc, class RangeX, class RangeY>
inline void MakeDataObjectCommon(OutputFunc output_func, MatrixData::MatRef map, RangeX rx, RangeY ry)
{
	//uint32_t xsize = map.GetSize(0);
	//uint32_t ysize = map.GetSize(1);
	auto [xsize, ysize] = map.GetSize();
	typename RangeY::iterator ity(ry);
	//xsize、ysizeはxcoord.size()-1、ycoord.size()-1にそれぞれ等しいはず。
	for (uint32_t iy = 0; iy < ysize && !ity.End(); ++iy, ++ity)
	{
		auto [y, cy] = *ity;
		typename RangeX::iterator itx(rx);
		for (uint32_t ix = 0; ix < xsize; ++ix, ++itx)
		{
			if (itx.End())
			{
				std::cout <<
					"WARNING : The numbers of datapoints are inconsistent."
					" The trailing data points are truncated to fit the smallest data set." << std::endl;
			}
			auto [x, cx] = *itx;
			map.Out(output_func, x, y, cx, cy, ix, iy);
		}
		auto [x, cx] = *itx;
		output_func(x, y, cx, cy, " 0\n");
	}
	typename RangeX::iterator itx(rx);
	auto [y, cy] = *ity;
	for (uint32_t ix = 0; ix < xsize && !itx.End(); ++ix, ++itx)
	{
		auto [x, cx] = *itx;
		output_func(x, y, cx, cy, " 0");
	}
	auto [x, cx] = *itx;
	output_func(x, y, cx, cy, " 0");
}
template <class ...Args>
inline void MakeDataObject(GPMCanvas* g, const std::string& name, Args&& ...args)
{
	if (g->IsInMemoryDataTransferEnabled()) 
	{
		// make datablock
		g->Command(name + " << EOD");
		ToFPtr output_func{ g->GetPipe() };
		MakeDataObjectCommon(output_func, std::forward<Args>(args)...);
		g->Command("EOD");
	}
	else
	{
		// make file
		std::ofstream ofs(name);
		if (!ofs) throw InvalidArg("file \"" + name + "\" cannot open.");
		ToOStr output_func{ ofs };
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
		std::string style;
		std::string others;
		switch (p.mStyle)
		{
		case Style::lines: style = " lines"; break;
		case Style::impulses: style = " impulses"; break;
		case Style::steps: style = " steps"; break;
		case Style::fsteps: style = " fsteps"; break;
		case Style::histeps: style = " histeps"; break;
		case Style::boxes: style = " boxes"; break;
		default: break;
		}
		if (p.mLineType != -2) others += " linetype " + std::to_string(p.mLineType);
		if (p.mLineWidth != -1) others += " linewidth " + std::to_string(p.mLineWidth);
		if (!p.mColor.empty()) others += " linecolor '" + p.mColor + "'";
		else if (p.mVariableColor) others += " linecolor palette";

		if (p.mStyle == Style::boxes || p.mStyle == Style::steps)
		{
			if (p.mStyle == Style::steps) style = " fillsteps";
			//現状、fill系オプションはboxesまたはstepsにしか使えない。
			if (!p.mFillColor.empty()) others += " fillcolor '" + p.mFillColor + "'";
			else if (p.mVariableFillColor) others += " fillcolor palette";
			{
				std::string fs;
				if (p.mTransparent) fs += " transparent";
				if (p.mSolid != -1) fs += Format(" solid %lf", p.mSolid);
				else if (p.mPattern != -1) fs += Format(" pattern %d", p.mPattern);
				if (!fs.empty()) others += " fs" + fs;
			}
			{
				std::string bd;
				if (p.mBorderType == -2) bd += Format(" noborder");
				else if (p.mBorderType != -3) bd += Format(" %d", p.mBorderType);
				if (!p.mBorderColor.empty()) bd += " linecolor '" + p.mBorderColor + "'";
				if (!bd.empty()) others += " border" + bd;
			}
		}
		c += style;
		c += others;
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
		if ((v.mArrowHead & 0b11) == 0) c += " head";
		else if ((v.mArrowHead & 0b11) == 1) c += " heads";
		else if ((v.mArrowHead & 0b11) == 2) c += " noheads";
		if ((v.mArrowHead & 0b1100) == 0) c += " filled";
		else if ((v.mArrowHead & 0b1100) == 4) c += " empty";
		else if ((v.mArrowHead & 0b1100) == 8) c += " nofilled";
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
std::string ColormapPlotCommand(const GraphParam&)
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
	void SetFormat##AXIS(const std::string& fmt);\
	void Set##AXIS##DataTime(const std::string& fmt = std::string());\
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
inline void GPM##AXIS##Axis<GPM>::SetFormat##AXIS(const std::string& fmt) { this->SetFormat(axis, fmt); }\
template <class GPM>\
inline void GPM##AXIS##Axis<GPM>::Set##AXIS##DataTime(const std::string& fmt) { this->SetDataTime(axis, fmt); }\
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
struct FillOption : public BaseOption {};
struct LineOption : public BaseOption {};
struct PointOption : public LineOption, public FillOption {};
struct VectorOption : public LineOption {};
struct FilledCurveOption : public FillOption {};

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(x, detail::ArrayData, BaseOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(y, detail::ArrayData, BaseOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(title, std::string_view, BaseOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(axis, std::string_view, BaseOption)//y2軸を使いたい場合などに、"x1y2"のように指定する。

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(linetype, int, LineOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(linewidth, double, LineOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(color, std::string_view, LineOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(variable_color, detail::ArrayData, LineOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(style, Style, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(pointtype, int, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(pointsize, double, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(smooth, Smooth, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xerrorbar, detail::ArrayData, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(yerrorbar, detail::ArrayData, PointOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(variable_size, detail::ArrayData, PointOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xlen, detail::ArrayData, VectorOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(ylen, detail::ArrayData, VectorOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(arrowhead, int, VectorOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(fillpattern, int, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(fillsolid, double, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION(filltransparent, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(fillcolor, std::string_view, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(variable_fillcolor, std::string_view, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(bordercolor, std::string_view, FillOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(bordertype, int, FillOption)//linetype

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(ybelow, detail::ArrayData, FilledCurveOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(baseline, std::string_view, FilledCurveOption)
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
		mPattern(-1), mSolid(-1), mTransparent(false), mBorderType(-3),
		mStyle(Style::none), mPointType(-1), mPointSize(-1), mSmooth(Smooth::none)
	{}

	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		mLineType = GetKeywordArg(plot::linetype, ops..., -2);
		mLineWidth = GetKeywordArg(plot::linewidth, ops..., -1);
		mColor = GetKeywordArg(plot::color, ops..., "");

		mFillColor = GetKeywordArg(plot::fillcolor, ops..., "");
		if (KeywordExists(plot::variable_fillcolor, ops...)) mVariableFillColor = GetKeywordArg(plot::variable_fillcolor, ops...);
		mPattern = GetKeywordArg(plot::fillpattern, ops..., -1);
		mSolid = GetKeywordArg(plot::fillsolid, ops..., -1);
		mTransparent = KeywordExists(plot::filltransparent, ops...);
		mBorderColor = GetKeywordArg(plot::bordercolor, ops..., "");
		mBorderType = GetKeywordArg(plot::bordertype, ops..., -3);

		if (KeywordExists(plot::xerrorbar, ops...)) mXErrorbar = GetKeywordArg(plot::xerrorbar, ops...);
		if (KeywordExists(plot::yerrorbar, ops...)) mYErrorbar = GetKeywordArg(plot::yerrorbar, ops...);
		if (KeywordExists(plot::variable_color, ops...)) mVariableColor = GetKeywordArg(plot::variable_color, ops...);
		if (KeywordExists(plot::variable_size, ops...)) mVariableSize = GetKeywordArg(plot::variable_size, ops...);
		mStyle = GetKeywordArg(plot::style, ops..., Style::points);
		mSmooth = GetKeywordArg(plot::smooth, ops..., Smooth::none);
		mPointType = GetKeywordArg(plot::pointtype, ops..., -1);
		mPointSize = GetKeywordArg(plot::pointsize, ops..., -1);
	}

	//LineOption
	int mLineType;//-2ならデフォルト
	double mLineWidth;//-1ならデフォルト、-2ならvariable
	std::string mColor;
	detail::ArrayData mVariableColor;

	//FillOption
	std::string mFillColor;
	detail::ArrayData mVariableFillColor;
	int mPattern;
	double mSolid;
	bool mTransparent;
	std::string mBorderColor;
	int mBorderType;//-2はnorborderを意味する。

	//PointOption
	Style mStyle;//デフォルトではPOINTS
	int mPointType;//-1ならデフォルト
	double mPointSize;//-1ならデフォルト、-2ならvariable
	Smooth mSmooth;
	detail::ArrayData mX;
	detail::ArrayData mY;
	detail::ArrayData mXErrorbar;
	detail::ArrayData mYErrorbar;
	detail::ArrayData mVariableSize;
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
	detail::ArrayData mVariableColor;

	//VectorOption
	detail::ArrayData mX;
	detail::ArrayData mY;
	detail::ArrayData mXLen;
	detail::ArrayData mYLen;
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
	detail::ArrayData mVariableColor;
	std::string mBaseline;
	int mPattern;
	double mSolid;
	bool mTransparent;
	std::string mBorderColor;
	int mBorderType;//-2はnorborderを意味する。

	//FilledCurveOption
	detail::ArrayData mX;
	detail::ArrayData mY;
	detail::ArrayData mY2;
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

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotPoints(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	GPMPlotBuffer2D PlotLines(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	GPMPlotBuffer2D PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
								RangeAdapter xlen, RangeAdapter ylen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	GPMPlotBuffer2D PlotVectors(std::string_view filename,
								std::string_view xbegin, std::string_view xlen,
								std::string_view ybegin, std::string_view ylen,
								Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(RangeAdapter x, RangeAdapter y, RangeAdapter y2,
									 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	GPMPlotBuffer2D PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, std::string_view y2,
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
	using detail::GPM2DAxis<GPMCanvas>::GPM2DAxis;

	friend class GPMMultiPlotter;

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotPoints(std::string_view equation, Options ...ops);
	
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::PointOption)>
	_Buffer PlotLines(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	_Buffer PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
						RangeAdapter xlen, RangeAdapter ylen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::VectorOption)>
	_Buffer PlotVectors(std::string_view filename,
						std::string_view xbegin, std::string_view xlen,
						std::string_view ybegin, std::string_view ylen,
						Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(RangeAdapter x, RangeAdapter y, RangeAdapter y2,
							 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::FilledCurveOption)>
	_Buffer PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, std::string_view y2,
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
		auto GET_ARRAY = [this](ArrayData& X, std::string_view ax,
							std::vector<ArrayData::Range>& it, std::vector<std::string>& column, std::string& labelcolumn)
		{
			switch (X.GetType())
			{
			case ArrayData::NUM_RANGE:
				it.emplace_back(X.GetRange());
				column.emplace_back(std::to_string(it.size()));
				break;
			case ArrayData::STR_RANGE:
				it.emplace_back(X.GetRange());
				if (mCanvas->IsDateTimeEnabled(ax))
					column.emplace_back(std::to_string(it.size()));
				else
					labelcolumn = Format("%stic(%d)", ax, it.size());
				break;
			case ArrayData::COLUMN:
				column.emplace_back(X.GetColumn());
				break;
			case ArrayData::UNIQUE:
				column.emplace_back(Format("($1-$1+%lf)", X.GetUnique()));
				break;
			}
		};

		std::vector<ArrayData::Range> it;
		std::vector<std::string> column;
		std::string labelcolumn;

		auto find_axis = [&i](const char* a) { return i.mAxis.find(a) != std::string::npos; };
		std::string x_x2 = find_axis("x2") ? mCanvas->Command("set x2tics"), "x2" : "x";
		std::string y_y2 = find_axis("y2") ? mCanvas->Command("set y2tics"), "y2" : "y";
		//ファイルを作成する。
		if (i.IsPoint())
		{
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");

			GET_ARRAY(p.mX, x_x2, it, column, labelcolumn);
			GET_ARRAY(p.mY, y_y2, it, column, labelcolumn);

			if (p.mXErrorbar) GET_ARRAY(p.mXErrorbar, x_x2, it, column, labelcolumn);
			if (p.mYErrorbar) GET_ARRAY(p.mYErrorbar, y_y2, it, column, labelcolumn);
			if (p.mVariableColor) GET_ARRAY(p.mVariableColor, "cb", it, column, labelcolumn);
			if (p.mVariableSize) GET_ARRAY(p.mVariableSize, "variable_size", it, column, labelcolumn);
		}
		else if (i.IsVector())
		{
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(v.mX, x_x2, it, column, labelcolumn);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(v.mY, y_y2, it, column, labelcolumn);
			if (!v.mXLen) throw InvalidArg("xlen list is not given.");
			GET_ARRAY(v.mXLen, x_x2, it, column, labelcolumn);
			if (!v.mYLen) throw InvalidArg("ylen list is not given.");
			GET_ARRAY(v.mYLen, y_y2, it, column, labelcolumn);

			if (v.mVariableColor) GET_ARRAY(v.mVariableColor, "variable_color", it, column, labelcolumn);
		}
		else if (i.IsFilledCurve())
		{
			auto& f = i.GetFilledCurveParam();
			if (!f.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(f.mX, x_x2, it, column, labelcolumn);
			if (!f.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(f.mY, y_y2, it, column, labelcolumn);

			if (f.mY2) GET_ARRAY(f.mY2, y_y2, it, column, labelcolumn);
			if (f.mVariableColor) GET_ARRAY(f.mVariableColor, "variable_fillcolor", it, column, labelcolumn);
		}
		MakeDataObject(mCanvas, i.mGraph, it);
		if (!labelcolumn.empty()) column.emplace_back(std::move(labelcolumn));
		i.mColumn = std::move(column);
	}
	else if (i.mType == GraphParam::FILE)
	{
		auto ADD_COLUMN = [](const ArrayData& a, const std::string& name, std::vector<std::string>& c)
		{
			if (a.GetType() == ArrayData::COLUMN) c.emplace_back(a.GetColumn());
			else if (a.GetType() == ArrayData::UNIQUE) c.emplace_back("($1-$1+" + std::to_string(a.GetUnique()) + ")");
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops)
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
PlotPoints(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops)
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
PlotPoints(std::string_view equation, Options ...ops)
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
PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	return PlotPoints(x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops)
{
	return PlotPoints(filename, xcol, ycol, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotLines(std::string_view equation, Options ...ops)
{
	return PlotPoints(equation, plot::style = Style::lines, ops...);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
			RangeAdapter xlen, RangeAdapter ylen,
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
PlotVectors(std::string_view filename,
			std::string_view xfrom, std::string_view xlen,
			std::string_view yfrom, std::string_view ylen,
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(RangeAdapter x, RangeAdapter y, Options ...ops)
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
PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, Options ...ops)
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBuffer2D<GraphParam> GPMPlotBuffer2D<GraphParam>::
PlotFilledCurves(RangeAdapter x, RangeAdapter y, RangeAdapter y2,
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
PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, std::string_view y2, Options ...ops)
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(filename, xcol, ycol, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotPoints(std::string_view equation, Options ...ops)
{
	_Buffer r(this);
	return r.PlotPoints(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(std::string_view filename, std::string_view xcol, std::string_view ycol, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(filename, xcol, ycol, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotLines(std::string_view equation, Options ...ops)
{
	_Buffer r(this);
	return r.PlotLines(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
			RangeAdapter xlen, RangeAdapter ylen,
			Options ...ops)
{
	_Buffer r(this);
	return r.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotVectors(std::string_view filename,
			std::string_view xfrom, std::string_view yfrom,
			std::string_view xlen, std::string_view ylen,
			Options ...ops)
{
	_Buffer r(this);
	return r.PlotVectors(filename, xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(filename, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(RangeAdapter x, RangeAdapter y, RangeAdapter y2,
				 Options ...ops)
{
	_Buffer r(this);
	return r.PlotFilledCurves(x, y, y2, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvas2D<GraphParam, Buffer>::
PlotFilledCurves(std::string_view filename, std::string_view x, std::string_view y, std::string_view y2, Options ...ops)
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

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(z, detail::ArrayData, Base3DOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(zlen, detail::ArrayData, Vector3DOption)

CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(map, const detail::MatrixData&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(xcoord, const detail::ArrayData&, ColormapOption)
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(ycoord, const detail::ArrayData&, ColormapOption)
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
CUF_DEFINE_TAGGED_KEYWORD_OPTION_WITH_VALUE(cntrcolor, std::string_view, ColormapOption)
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
	detail::ArrayData mZ;
};
struct GPMVectorParamCM : public GPMVectorParam
{
	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		GPMVectorParam::SetOptions(ops...);
	}
	detail::ArrayData mZ;
	detail::ArrayData mZLen;
};
struct GPMFilledCurveParamCM : public GPMFilledCurveParam
{
	template <class ...Ops>
	void SetOptions(Ops ...ops)
	{
		GPMFilledCurveParam::SetOptions(ops...);
	}
	detail::ArrayData mZ;
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
	ArrayData mXCoord;
	ArrayData mYCoord;
	std::pair<double, double> mXRange;
	std::pair<double, double> mYRange;
	MatrixData mZMap;

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
	GPMPlotBufferCM PlotPoints(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(std::string_view filename,
							   std::string_view x, std::string_view y, std::string_view z,
							   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(std::string_view filename,
							   std::string_view x, std::string_view y,
							   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotPoints(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(std::string_view filename,
							  std::string_view x, std::string_view y, std::string_view z,
							  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(std::string_view filename,
							  std::string_view x, std::string_view y,
							  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	GPMPlotBufferCM PlotLines(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom, RangeAdapter zfrom,
								RangeAdapter xlen, RangeAdapter ylen, RangeAdapter zlen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(std::string_view filename,
								std::string_view xfrom, std::string_view yfrom, std::string_view zfrom,
								std::string_view xlen, std::string_view ylen, std::string_view zlen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
								RangeAdapter xlen, RangeAdapter ylen,
								Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	GPMPlotBufferCM PlotVectors(std::string_view filename,
								std::string_view xfrom, std::string_view yfrom,
								std::string_view xlen, std::string_view ylen,
								Options ...ops);

	template <class Type, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const Matrix<Type>& map, RangeAdapter x, RangeAdapter y,
								 Options ...ops);
	template <class Type, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(const Matrix<Type>& map, std::pair<double, double> x, std::pair<double, double> y,
								 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(std::string_view filename, std::string_view z, std::string_view x, std::string_view y,
								 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	GPMPlotBufferCM PlotColormap(std::string_view equation, Options ...ops);

protected:

	GPMPlotBufferCM Plot(GraphParam& i);

	static std::string PlotCommand(const GraphParam& i, const bool IsInMemoryDataTransferEnabled);
	static std::string InitCommand();

	std::vector<GraphParam> mParam;
	GPMCanvas* mCanvas;
};


template <class GraphParam, template <class> class Buffer>
class GPMCanvasCM : public detail::GPM2DAxis<GPMCanvas>
{
public:

	using _Buffer = Buffer<GraphParam>;

	GPMCanvasCM(const std::string& output, double sizex = 0., double sizey = 0.);
	GPMCanvasCM();

	friend class GPMMultiPlotter;

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(std::string_view filename,
					   std::string_view x, std::string_view y, std::string_view z,
					   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(std::string_view filename,
					   std::string_view x, std::string_view y,
					   Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotPoints(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(std::string_view filename,
					  std::string_view x, std::string_view y, std::string_view z,
					  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(std::string_view filename,
					  std::string_view x, std::string_view y,
					  Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Point3DOption)>
	_Buffer PlotLines(std::string_view equation, Options ...ops);

	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom, RangeAdapter zfrom,
						RangeAdapter xlen, RangeAdapter ylen, RangeAdapter zlen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(std::string_view filename,
						std::string_view xfrom, std::string_view yfrom, std::string_view zfrom,
						std::string_view xlen, std::string_view ylen, std::string_view zlen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
						RangeAdapter xlen, RangeAdapter ylen,
						Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::Vector3DOption)>
	_Buffer PlotVectors(std::string_view filename,
						std::string_view xfrom, std::string_view yfrom,
						std::string_view xlen, std::string_view ylen,
						Options ...ops);

	template <class Type, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const Matrix<Type>& map, RangeAdapter x, RangeAdapter y,
						 Options ...ops);
	template <class Type, class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(const Matrix<Type>& map, std::pair<double, double> x, std::pair<double, double> y,
						 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(std::string_view filename, std::string_view z, std::string_view x, std::string_view y,
						 Options ...ops);
	template <class ...Options, CUF_TAGGED_ARGS_ENABLER(Options, plot::ColormapOption)>
	_Buffer PlotColormap(std::string_view equation, Options ...ops);

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
struct CoordRange
{
	struct iterator
	{
		iterator(const CoordRange& p)
			: parent(&p), min(0.), center(0.), pos(-1)
		{
			if (parent->x.End())
				throw InvalidArg("The number of datapoints for colormap must be greater than 1.");
			double curr = *parent->x;
			++parent->x;
			if (parent->x.End())
				throw InvalidArg("The number of datapoints for colormap must be greater than 1.");
			double next = *parent->x;
			min = curr - (next - curr) / 2.;
			center = curr;
		}
		void operator++()
		{
			if (pos == 1) throw OutOfRange("");//これが投げられたらバグ。
			if (parent->x.End())
			{
				//xは常にひとつ先の座標を指している。
				//よって、このスコープは最後の、つまりMatrixの端のさらに一つ次の座標へ移動しようとしている。
				pos = 1;
				double prev = center;
				double w_per_2 = center - min;
				min = center + w_per_2;
				center = prev + w_per_2 * 2;
			}
			else
			{
				++parent->x;
				pos = 0;
				double prev = center;
				double curr = *parent->x;
				min = (curr + prev) / 2.;
				center = curr;
			}
		}
		bool End() const { return pos == 1; }
		std::pair<double, double> operator*() const
		{
			return { min, center };
		}

	private:
		const CoordRange* parent;
		double min, center;
		int pos;//-1:first, 0:middle, 1:last
	};
	CoordRange(ArrayData::Range& r)
		: x(r)
	{}
	ArrayData::Range& x;
};
struct CoordMinMax
{
	struct iterator
	{
		iterator(const CoordMinMax& p)
			: current(0), parent(&p) {}
		void operator++() { ++current; }
		bool End() const { return current == parent->size; }
		std::pair<double, double> operator*() const
		{
			return std::make_pair(parent->min + current * parent->width,
								  parent->cmin + current * parent->width);
		}
	private:
		size_t current;
		const CoordMinMax* parent;
	};
	CoordMinMax(std::pair<double, double> r, size_t s)
	{
		size = s;
		width = (r.second - r.first) / double(size - 1);
		min = r.first - width / 2.;
		cmin = r.first;
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
		auto GET_ARRAY = [this](ArrayData& X, std::string_view ax,
								std::vector<ArrayData::Range>& it, std::vector<std::string>& column, std::string& labelcolumn)
		{
			switch (X.GetType())
			{
			case ArrayData::NUM_RANGE:
				it.emplace_back(X.GetRange());
				column.emplace_back(std::to_string(it.size()));
				break;
			case ArrayData::STR_RANGE:
				it.emplace_back(X.GetRange());
				if (mCanvas->IsDateTimeEnabled(ax))
					column.emplace_back(std::to_string(it.size()));
				else
					labelcolumn = Format("%stic(%d)", ax, it.size());
				break;
			case ArrayData::COLUMN:
				column.emplace_back(X.GetColumn());
				break;
			case ArrayData::UNIQUE:
				column.emplace_back(Format("($1-$1+%lf)", X.GetUnique()));
				break;
			}
		};

		std::vector<std::string> column;
		std::string labelcolumn;

		auto find_axis = [&i](const char* a) { return i.mAxis.find(a) != std::string::npos; };
		std::string x_x2 = find_axis("x2") ? mCanvas->Command("set x2tics"), "x2" : "x";
		std::string y_y2 = find_axis("y2") ? mCanvas->Command("set y2tics"), "y2" : "y";
		std::string z_z2 = find_axis("z2") ? mCanvas->Command("set z2tics"), "z2" : "z";

		//ファイルを作成する。
		if (i.IsColormap())
		{
			auto& m = i.GetColormapParam();
			if (!m.mZMap) throw InvalidArg("z map is not given");
			if (m.mZMap.GetType() != MatrixData::NUM_MAT)
				throw InvalidArg("z map in the data plot mode must be given in the form of Matrix<double>.");

			//mapがMatrixであるとき、x、yの座標値も配列かrangeで与えられていなければならない。
			if (!((m.mXCoord && m.mXCoord.GetType() == ArrayData::NUM_RANGE) ||
				  m.mXRange != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");
			if (!((m.mYCoord && m.mYCoord.GetType() == ArrayData::NUM_RANGE) ||
				  m.mYRange != std::make_pair(DBL_MAX, -DBL_MAX))) throw InvalidArg("");

			auto [xsize, ysize] = m.mZMap.GetMatrix().GetSize();

			column = { "1", "2", "5" };
			if (m.mXCoord)
			{
				auto x = m.mXCoord.GetRange();
				if (m.mYCoord)
				{
					auto y = m.mYCoord.GetRange();
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), CoordRange(x), CoordRange(y));
				}
				else
				{
					auto y = m.mYRange;
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), CoordRange(x), CoordMinMax(y, ysize));
				}
			}
			else
			{
				auto x = m.mXRange;
				if (m.mYCoord)
				{
					auto y = m.mYCoord.GetRange();
					//if (y.size() != ysize) throw InvalidArg("size of y coordinate list and the y size of mat must be the same.");
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), CoordMinMax(x, xsize), CoordRange(y));
				}
				else
				{
					auto y = m.mYRange;
					MakeDataObject(mCanvas, i.mGraph, m.mZMap.GetMatrix(), CoordMinMax(x, xsize), CoordMinMax(y, ysize));
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
			std::vector<ArrayData::Range> it;
			auto& p = i.GetPointParam();
			if (!p.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(p.mX, x_x2, it, column, labelcolumn);
			if (!p.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(p.mY, y_y2, it, column, labelcolumn);
			if (!p.mZ) throw InvalidArg("z coordinate list is not given.");
			GET_ARRAY(p.mZ, z_z2, it, column, labelcolumn);

			if (p.mXErrorbar)
			{
				GET_ARRAY(p.mXErrorbar, x_x2, it, column, labelcolumn);
			}
			if (p.mYErrorbar)
			{
				GET_ARRAY(p.mYErrorbar, y_y2, it, column, labelcolumn);
			}
			if (p.mVariableColor)
			{
				GET_ARRAY(p.mVariableColor, "cb", it, column, labelcolumn);
			}
			if (p.mVariableSize)
			{
				GET_ARRAY(p.mVariableSize, "variable_size", it, column, labelcolumn);
			}
			MakeDataObject(mCanvas, i.mGraph, it);
		}
		else if (i.IsVector())
		{
			std::vector<ArrayData::Range> it;
			auto& v = i.GetVectorParam();
			if (!v.mX) throw InvalidArg("x coordinate list is not given.");
			GET_ARRAY(v.mX, x_x2, it, column, labelcolumn);
			if (!v.mY) throw InvalidArg("y coordinate list is not given.");
			GET_ARRAY(v.mY, y_y2, it, column, labelcolumn);
			if (!v.mZ) throw InvalidArg("z coordinate list is not given.");
			GET_ARRAY(v.mZ, z_z2, it, column, labelcolumn);
			if (!v.mXLen) throw InvalidArg("xlen list is not given.");
			GET_ARRAY(v.mXLen, x_x2, it, column, labelcolumn);
			if (!v.mYLen) throw InvalidArg("ylen list is not given.");
			GET_ARRAY(v.mYLen, y_y2, it, column, labelcolumn);
			if (!v.mZLen) throw InvalidArg("zlen list is not given.");
			GET_ARRAY(v.mZLen, z_z2, it, column, labelcolumn);

			if (v.mVariableColor)
			{
				GET_ARRAY(v.mVariableColor, "variable_color", it, column, labelcolumn);
			}
			MakeDataObject(mCanvas, i.mGraph, it);
		}
		if (!labelcolumn.empty()) column.emplace_back(std::move(labelcolumn));
		i.mColumn = std::move(column);
	}
	else if (i.mType == GraphParam::FILE)
	{
		auto ADD_COLUMN = [](const ArrayData& a, const std::string& name, std::vector<std::string>& c)
		{
			if (a.GetType() == ArrayData::COLUMN) c.emplace_back(a.GetColumn());
			else if (a.GetType() == ArrayData::UNIQUE) c.emplace_back("($1-$1+" + std::to_string(a.GetUnique()) + ")");
			else throw InvalidArg(name + "list in the file plot mode must be given in the form of the string column or unique value.");
		};
		std::vector<std::string> column;
		if (i.IsColormap())
		{
			auto& p = i.GetColormapParam();
			if (!p.mZMap) throw InvalidArg("z coordinate list is not given.");
			if (p.mZMap.GetType() == MatrixData::COLUMN) column.emplace_back(p.mZMap.GetColumn());
			else if (p.mZMap.GetType() == MatrixData::UNIQUE) column.emplace_back("($1-$1+" + std::to_string(p.mZMap.GetUnique()) + ")");
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
PlotPoints(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::DATA;
	i.SetBaseOptions(ops...);
	//point
	auto& p = i.GetPointParam();
	p.mX = x;
	p.mY = y;
	p.mZ = z;
	p.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(std::string_view filename,
		   std::string_view x, std::string_view y, std::string_view z,
		   Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

	auto& p = i.GetPointParam();
	p.mX = x;
	p.mY = y;
	p.mZ = z;
	p.SetOptions(ops...);
	return Plot(i);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops)
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
PlotPoints(std::string_view filename,
		   std::string_view x, std::string_view y,
		   Options ...ops)
{
	GraphParam i;
	i.AssignPoint();
	i.mType = GraphParam::FILE;
	i.mGraph = filename;
	i.SetBaseOptions(ops...);

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
PlotPoints(std::string_view equation, Options ...ops)
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
PlotLines(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops)
{
	return PlotPoints(x, y, z, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(std::string_view filename,
		  std::string_view x, std::string_view y, std::string_view z,
		  Options ...ops)
{
	return PlotPoints(filename, x, y, z, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	return PlotPoints(x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(std::string_view filename,
		  std::string_view x, std::string_view y,
		  Options ...ops)
{
	return PlotPoints(filename, x, y, plot::style = Style::lines, ops...);
}
template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotLines(std::string_view equation, Options ...ops)
{
	return PlotPoints(equation, plot::style = Style::lines, ops...);
}

template <class GraphParam>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom, RangeAdapter zfrom,
			RangeAdapter xlen, RangeAdapter ylen, RangeAdapter zlen,
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
PlotVectors(std::string_view filename,
			std::string_view xfrom, std::string_view yfrom, std::string_view zfrom,
			std::string_view xlen, std::string_view ylen, std::string_view zlen,
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
PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
			RangeAdapter xlen, RangeAdapter ylen,
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
PlotVectors(std::string_view filename,
			std::string_view xfrom, std::string_view yfrom,
			std::string_view xlen, std::string_view ylen,
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
template <class Type, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const Matrix<Type>& map, RangeAdapter x, RangeAdapter y,
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
template <class Type, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(const Matrix<Type>& map, std::pair<double, double> x, std::pair<double, double> y,
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
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline GPMPlotBufferCM<GraphParam> GPMPlotBufferCM<GraphParam>::
PlotColormap(std::string_view filename, std::string_view z, std::string_view x, std::string_view y,
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
PlotColormap(std::string_view equation, Options ...ops)
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
PlotPoints(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(std::string_view filename,
		   std::string_view x, std::string_view y, std::string_view z,
		   Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(filename, x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(std::string_view filename,
		   std::string_view x, std::string_view y,
		   Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(filename, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotPoints(std::string_view equation, Options ...ops)
{
	_Buffer p(this);
	return p.PlotPoints(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(RangeAdapter x, RangeAdapter y, RangeAdapter z, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(std::string_view filename,
		   std::string_view x, std::string_view y, std::string_view z,
		   Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(filename, x, y, z, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(RangeAdapter x, RangeAdapter y, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(std::string_view filename,
		  std::string_view x, std::string_view y,
		  Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(filename, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotLines(std::string_view equation, Options ...ops)
{
	_Buffer p(this);
	return p.PlotLines(equation, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom, RangeAdapter zfrom,
			RangeAdapter xlen, RangeAdapter ylen, RangeAdapter zlen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(xfrom, yfrom, zfrom, xlen, ylen, zlen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(std::string_view filename,
			std::string_view xfrom, std::string_view yfrom, std::string_view zfrom,
			std::string_view xlen, std::string_view ylen, std::string_view zlen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(filename, xfrom, yfrom, zfrom, xlen, ylen, zlen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(RangeAdapter xfrom, RangeAdapter yfrom,
			RangeAdapter xlen, RangeAdapter ylen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(xfrom, yfrom, xlen, ylen, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
inline Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotVectors(std::string_view filename,
			std::string_view xfrom, std::string_view yfrom,
			std::string_view xlen, std::string_view ylen,
			Options ...ops)
{
	_Buffer p(this);
	return p.PlotVectors(filename, xfrom, yfrom, xlen, ylen, ops...);
}

template <class GraphParam, template <class> class Buffer>
template <class Type, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(const Matrix<Type>& map, RangeAdapter x, RangeAdapter y,
			 Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(map, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class Type, class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(const Matrix<Type>& map, std::pair<double, double> x, std::pair<double, double> y,
			 Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(map, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(std::string_view filename, std::string_view z, std::string_view x, std::string_view y,
			 Options ...ops)
{
	_Buffer p(this);
	return p.PlotColormap(filename, z, x, y, ops...);
}
template <class GraphParam, template <class> class Buffer>
template <class ...Options, bool B, std::enable_if_t<B, std::nullptr_t>>
Buffer<GraphParam> GPMCanvasCM<GraphParam, Buffer>::
PlotColormap(std::string_view equation, Options ...ops)
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

}

}

#endif
