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
	ArrayData() {}
	ArrayData(const std::vector<double>& vector) : mVariant(&vector) {}
	ArrayData(const std::string& column) : mVariant(&column) {}
	ArrayData(double value) : mVariant(value) {}

	bool IsEmpty() const { return mVariant.IsEmpty(); }
	int GetType() const { return mVariant.GetIndex(); }
	const std::vector<double>& GetVector() const { return *mVariant.Get<0>(); }
	const std::string& GetColumn() const { return *mVariant.Get<1>(); }
	double GetValue() const { return mVariant.Get<2>(); }

	operator bool() const { return !IsEmpty(); }

private:

	Variant<const std::vector<double>*, const std::string*, double> mVariant;
};
struct MatrixData
{
	MatrixData() {}
	MatrixData(const Matrix<double>& matrix) : mVariant(&matrix) {}
	MatrixData(const std::string& column) : mVariant(&column) {}
	MatrixData(double value) : mVariant(value) {}

	bool IsEmpty() const { return mVariant.IsEmpty(); }
	int GetType() const { return mVariant.GetIndex(); }
	const Matrix<double>& GetMatrix() const { return *mVariant.Get<0>(); }
	const std::string& GetColumn() const { return *mVariant.Get<1>(); }
	double GetValue() const { return mVariant.Get<2>(); }

	operator bool() const { return !IsEmpty(); }

private:

	Variant<const Matrix<double>*, const std::string*, double> mVariant;
};

}

}

}

#endif