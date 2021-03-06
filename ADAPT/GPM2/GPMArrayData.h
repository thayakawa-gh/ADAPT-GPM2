#ifndef GPM2_GPMARRAYDATA_H
#define GPM2_GPMARRAYDATA_H

#include <ADAPT/CUF/Matrix.h>
#include <ADAPT/CUF/Variant.h>
#include <vector>
#include <string>

namespace adapt
{

namespace gpm2
{

namespace plot
{

struct ArrayData
{
	enum Type { DBLVEC, STRVEC, COLUMN, UNIQUE, };
	ArrayData() {}
	ArrayData(const std::vector<double>& vector) : mVariant(&vector) {}
	ArrayData(const std::vector<std::string>& strvec) : mVariant(&strvec) {}
	ArrayData(const std::string& column) : mVariant(column) {}
	ArrayData(const char* column) : mVariant(column) {}
	ArrayData(double value) : mVariant(value) {}

	bool IsEmpty() const { return mVariant.IsEmpty(); }
	Type GetType() const { return (Type)mVariant.GetIndex(); }
	const std::vector<double>& GetVector() const { return *mVariant.Get<DBLVEC>(); }
	const std::vector<std::string>& GetStrVec() const { return *mVariant.Get<STRVEC>(); }
	const std::string& GetColumn() const { return mVariant.Get<COLUMN>(); }
	double GetValue() const { return mVariant.Get<UNIQUE>(); }

	operator bool() const { return !IsEmpty(); }

private:

	Variant<const std::vector<double>*, const std::vector<std::string>*, std::string, double> mVariant;
};
struct MatrixData
{
	enum Type { DBLMAT, COLUMN, UNIQUE, };

	MatrixData() {}
	MatrixData(const Matrix<double>& matrix) : mVariant(&matrix) {}
	MatrixData(const std::string& column) : mVariant(column) {}
	MatrixData(const char* column) : mVariant(column) {}
	MatrixData(double value) : mVariant(value) {}

	bool IsEmpty() const { return mVariant.IsEmpty(); }
	Type GetType() const { return (Type)mVariant.GetIndex(); }
	const Matrix<double>& GetMatrix() const { return *mVariant.Get<0>(); }
	const std::string& GetColumn() const { return mVariant.Get<1>(); }
	double GetValue() const { return mVariant.Get<2>(); }

	operator bool() const { return !IsEmpty(); }

private:

	Variant<const Matrix<double>*, std::string, double> mVariant;
};

}

}

}

#endif