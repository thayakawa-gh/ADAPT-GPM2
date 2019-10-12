//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#ifndef CUF_MATRIX_H
#define CUF_MATRIX_H

#include <memory>
#include <cassert>
#include <array>
#include <ADAPT/CUF/Template.h>

namespace adapt
{

inline namespace cuf
{

template <class T, int Dim = 2>
class Matrix
{
public:

	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, const T& i)
		: mSize({ x, y }), mMatrixData(std::make_unique<T[]>((size_t)x * (size_t)y))
	{
		auto* p = mMatrixData.get();
		auto* end = p + x * y;
		for (; p < end; ++p) *p = i;
	}
	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y)
		: mSize({ x, y }), mMatrixData(std::make_unique<T[]>((size_t)x * (size_t)y))
	{}
	template <bool B = (Dim == 3), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, const T& i)
		: mSize({ x, y, z }), mMatrixData(std::make_unique<T[]>((size_t)x * (size_t)y * (size_t)z))
	{
		auto* p = mMatrixData.get();
		auto* end = p + x * y;
		for (; p < end; ++p) *p = i;
	}
	template <bool B = (Dim == 3), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z)
		: mSize({ x, y, z }), mMatrixData(std::make_unique<T[]>((size_t)x * (size_t)y * (size_t)z))
	{}
	template <bool B = (Dim == 4), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, uint32_t t, const T& i)
		: mSize({ x, y, z, t }), mMatrixData(std::make_unique<T[]>((size_t)x * (size_t)y * (size_t)z * (size_t)t))
	{
		auto* p = mMatrixData.get();
		auto* end = p + x * y;
		for (; p < end; ++p) *p = i;
	}
	template <bool B = (Dim == 4), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, uint32_t t)
		: mSize({ x, y, z, t }), mMatrixData(std::make_unique<T[]>((size_t)x * (size_t)y * (size_t)z * (size_t)t))
	{}

	Matrix(const Matrix<T, Dim>& m) : mSize(m.mSize)
	{
		size_t size = 1;
		for (int i = 0; i < Dim; ++i) size *= mSize[i];
		mMatrixData = std::make_unique<T[]>(size);
		T* it = mMatrixData.get();
		const T* it2 = m.mMatrixData.get();
		const T* end = it + size;
		for (; it != end; ++it) *it = *it2;
	}
	Matrix(Matrix<T, Dim>&& m) = default;

	Matrix<T, Dim>& operator=(const Matrix<T, Dim>& m)
	{
		mMatrixData.reset();
		mSize = m.mSize;
		size_t size = 1;
		for (int i = 0; i < Dim; ++i) size *= mSize[i];
		mMatrixData = std::make_unique<T[]>(size);
		T* it = mMatrixData.get();
		const T* it2 = m.mMatrixData.get();
		const T* end = it + size;
		for (; it != end; ++it) *it = *it2;
	}
	Matrix<T, Dim>& operator=(Matrix<T, Dim>&& m) = default;

	static constexpr int GetDimension() { return Dim; }
	uint32_t GetSize(uint32_t dim) const { return mSize[dim]; }

	template <int Dim, int N, class T, template <class> class Qualifier, bool B = (N == Dim - 1)>
	class MatPursuer
	{
	public:
		MatPursuer(Qualifier<Matrix<T, Dim>>& m, std::size_t row) : mMatrix(m), mRow(row) {}

		template <bool B = std::is_const<MatPursuer<Dim, N - 1, T, Qualifier>>::value>
		std::enable_if_t<!B, MatPursuer<Dim, N + 1, T, Qualifier>> operator[](uint32_t i)
		{
			assert(i >= 0 && i < mMatrix.mSize[N]);
			return MatPursuer<Dim, N + 1, T, Qualifier>(mMatrix, mRow * mMatrix.mSize[N] + i);
		}

		MatPursuer<Dim, N + 1, T, Qualifier> operator[](uint32_t i) const
		{
			assert(i >= 0 && i < mMatrix.mSize[N]);
			return MatPursuer<Dim, N + 1, T, Qualifier>(mMatrix, mRow * mMatrix.mSize[N] + i);
		}
	private:
		std::size_t mRow;
		Qualifier<Matrix<T, Dim>>& mMatrix;
	};
	template <int Dim, int N, class T, template <class> class Qualifier>
	class MatPursuer<Dim, N, T, Qualifier, true>
	{
	public:
		MatPursuer(Qualifier<Matrix<T, Dim>>& m, std::size_t row) : mMatrix(m), mRow(row) {}

		template <bool B = std::is_const<MatPursuer<Dim, N - 1, T, Qualifier>>::value>
		std::enable_if_t<!B, T&> operator[](uint32_t i)
		{
			assert(i >= 0 && i < mMatrix.mSize[N]);
			return mMatrix.mMatrixData[mRow * mMatrix.mSize[N] + i];
		}

		const T& operator[](uint32_t i) const
		{
			assert(i >= 0 && i < mMatrix.mSize[N]);
			return mMatrix.mMatrixData[mRow * mMatrix.mSize[N] + i];
		}
	private:
		std::size_t mRow;
		Qualifier<Matrix<T, Dim>>& mMatrix;
	};

	//Dim>1のときはMatPursuerを返す。
	template <bool B = (Dim > 1)>
	std::enable_if_t<B, MatPursuer<Dim, 1, T, IdentityT>> operator[](uint32_t i)
	{
		assert(i >= 0 && i < mSize[0]);
		return MatPursuer<Dim, 1, T, IdentityT>(*this, i);
	}
	template <bool B = (Dim > 1)>
	std::enable_if_t<B, MatPursuer<Dim, 1, T, std::add_const_t>> operator[](uint32_t i) const
	{
		assert(i >= 0 && i < mSize[0]);
		return MatPursuer<Dim, 1, T, std::add_const_t>(*this, i);
	}

	//Dim == 1のときはMatPursuerを介す必要はない。
	template <bool B = (Dim == 1)>
	std::enable_if_t<B, T&> operator[](uint32_t i)
	{
		assert(i >= 0 && i < mSize[0]);
		return mMatrixData[i];
	}
	template <bool B = (Dim == 1)>
	std::enable_if_t<B, const T&> operator[](uint32_t i) const
	{
		assert(i >= 0 && i < mSize[0]);
		return mMatrixData[i];
	}

	//[0][0], [0][1], [0][2], ...みたいな順に走査する。はず。
	T* begin() { return mMatrixData.get(); }
	const T* begin() const { return mMatrixData.get(); }
	const T* cbegin() const { return begin(); }
	T* end() { return mMatrixData.get() + std::accumulate(mSize.begin(), mSize.end(), 1, std::multiplies<int>()); }
	const T* end() const { return mMatrixData.get() + std::accumulate(mSize.begin(), mSize.end(), 1, std::multiplies<int>()); }
	const T* cend() const { return end(); }

private:
	std::array<uint32_t, Dim> mSize;
	std::unique_ptr<T[]> mMatrixData;
};

}

}

#endif