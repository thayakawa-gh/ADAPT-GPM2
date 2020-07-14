#ifndef CUF_MATRIX_H
#define CUF_MATRIX_H

#include <memory>
#include <cassert>
#include <numeric>
#include <ADAPT/CUF/Template.h>
#include <ADAPT/CUF/Function.h>

namespace adapt
{

inline namespace cuf
{

template <class T, int Dim = 2>
class Matrix
{
	static constexpr size_t msRangeSize = (Dim + Dim % 2) * sizeof(uint32_t);
	struct Range
	{
		const uint32_t& operator[](int dim) const
		{
			assert(dim < Dim);
			return mSize[dim];
		}
		uint32_t& operator[](int dim)
		{
			assert(dim < Dim);
			return mSize[dim];
		}
		bool operator==(const Range& r) const
		{
			return Operator_eq(r, std::make_index_sequence<Dim>());
		}
		bool operator!=(const Range& r) const
		{
			return !(*this == r);
		}
		uint32_t* begin() { return mSize; }
		uint32_t* end() { return mSize + Dim; }
		const uint32_t* begin() const { return mSize; }
		const uint32_t* end() const { return mSize + Dim; }

		template <size_t ...Indices>
		bool Operator_eq(const Range& r, std::index_sequence<Indices...>) const
		{
			return AndAll((mSize[Indices] == r.mSize[Indices])...);
		}
		uint32_t mSize[Dim];
	};

public:

	Matrix()
		: mMatrixData(nullptr)
	{}

	template <bool B = (Dim == 1), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, const T& i)
		: mMatrixData(nullptr)
	{
		Allocate(x);
		GetRange() = { x };
		ConstructAll(x, i);
	}
	template <bool B = (Dim == 1), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x)
		: mMatrixData(nullptr)
	{
		Allocate(x);
		GetRange() = { x };
		ConstructAll(x, T());
	}
	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, const T& i)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y;
		Allocate(cap);
		GetRange() = { x, y };
		ConstructAll(cap, i);
	}
	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y;
		Allocate(cap);
		GetRange() = { x, y };
		ConstructAll(cap, T());
	}
	template <bool B = (Dim == 3), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, const T& i)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z;
		Allocate(cap);
		GetRange() = { x, y, z };
		ConstructAll(cap, i);
	}
	template <bool B = (Dim == 3), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z;
		Allocate(cap);
		GetRange() = { x, y, z };
		ConstructAll(cap, T());
	}
	template <bool B = (Dim == 4), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, uint32_t t, const T& i)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z * (size_t)t;
		Allocate(cap);
		GetRange() = { x, y, z, t };
		ConstructAll(cap, i);
	}
	template <bool B = (Dim == 4), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, uint32_t t)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z * (size_t)t;
		Allocate(cap);
		GetRange() = { x, y, z, t };
		ConstructAll(cap, T());
	}
	template <bool B = (Dim == 5), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, uint32_t t, uint32_t u, const T& i)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z * (size_t)t * (size_t)u;
		Allocate(cap);
		GetRange() = { x, y, z, t, u };
		ConstructAll(cap, i);
	}
	template <bool B = (Dim == 5), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(uint32_t x, uint32_t y, uint32_t z, uint32_t t, uint32_t u)
		: mMatrixData(nullptr)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z * (size_t)t * (size_t)u;
		Allocate(cap);
		GetRange() = { x, y, z, t, u };
		ConstructAll(cap, T());
	}

	template <bool B = (Dim == 1), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(std::initializer_list<T> a)
		: mMatrixData(nullptr)
	{
		size_t size = a.size();
		Allocate(size);
		GetRange() = { (uint32_t)size };
		auto* end = mMatrixData + size;
		auto* it = mMatrixData;
		for (auto x : a)
		{
			new (it) T(x);
			++it;
		}
	}
	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix(std::initializer_list<std::initializer_list<T>> a)
		: mMatrixData(nullptr)
	{
		size_t xsize = a.size();
		size_t ysize = a.begin()->size();
#ifndef NDEBUG
		for (auto x : a) assert(x.size() == ysize);
#endif
		Allocate(xsize * ysize);
		GetRange() = { (uint32_t)xsize, (uint32_t)ysize };
		auto* end = mMatrixData + xsize * ysize;
		auto* it = mMatrixData;
		for (auto x : a)
		{
			for (auto y : x)
			{
				new (it) T(y);
				++it;
			}
		}
	}

	Matrix(const Matrix& m)
		: mMatrixData(nullptr)
	{
		size_t cap = m.GetCapacity();
		Allocate(cap);
		GetRange() = m.GetRange();
		T* it = mMatrixData;
		const T* it2 = m.mMatrixData;
		const T* end = it + cap;
		for (; it != end; ++it, ++it2) new (it) T(*it2);
	}
	Matrix(Matrix&& m) noexcept
		: mMatrixData(m.mMatrixData)
	{
		m.mMatrixData = nullptr;
	}

	Matrix<T, Dim>& operator=(const Matrix& m)
	{
		size_t cap = m.GetCapacity();
		if (mMatrixData == nullptr) Allocate(cap);
		else if(GetCapacity() != cap) Reallocate(cap);
		else DestructAll(cap);

		{
			GetRange() = m.GetRange();
			T* it = mMatrixData;
			const T* it2 = m.mMatrixData;
			const T* end = it + cap;
			for (; it != end; ++it, ++it2) new (it) T(*it2);
		}
		return *this;
	}
	Matrix<T, Dim>& operator=(Matrix&& m)
	{
		mMatrixData = m.mMatrixData;
		m.mMatrixData = nullptr;
		return *this;
	}
	~Matrix()
	{
		Destroy();
	}

	template <bool B = (Dim == 1), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix& operator=(std::initializer_list<T> a)
	{
		size_t cap = a.size();
		if (mMatrixData == nullptr) Allocate(cap);
		else if (GetCapacity() != cap) Reallocate(cap);
		else DestructAll(cap);
		GetRange() = { (uint32_t)cap };
		auto* end = mMatrixData + cap;
		auto* it = mMatrixData;
		for (auto x : a)
		{
			new (it) T(x);
			++it;
		}
		return *this;
	}
	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	Matrix& operator=(std::initializer_list<std::initializer_list<T>> a)
	{
		size_t xsize = a.size();
		size_t ysize = a.begin()->size();
#ifndef NDEBUG
		for (auto x : a) assert(x.size() == ysize);
#endif
		size_t cap = xsize * ysize;
		if (mMatrixData == nullptr) Allocate(cap);
		else if (GetCapacity() != cap) Reallocate(cap);
		else DestructAll(cap);
		GetRange() = { (uint32_t)xsize, (uint32_t)ysize };
		auto* end = mMatrixData + cap;
		auto* it = mMatrixData;
		for (auto x : a)
		{
			for (auto y : x)
			{
				new (it) T(y);
				++it;
			}
		}
		return *this;
	}

	static constexpr int GetDimension() { return Dim; }
	uint32_t GetSize(uint32_t dim) const { return GetRange()[dim]; }
	size_t GetCapacity() const
	{
		size_t res = 1;
		for (int i = 0; i < Dim; ++i) res *= GetSize(i);
		return res;
	}
	//std::array<uint32_t, Dim> GetSizes() const { return mSize; }

	template <int N, template <class> class Qualifier, bool B = (N == Dim - 1)>
	class MatPursuer
	{
	public:
		MatPursuer(Qualifier<Matrix<T, Dim>>& m, std::size_t row) : mRow(row), mMatrix(m) {}

		template <bool C = std::is_const<MatPursuer<N - 1, Qualifier>>::value>
		std::enable_if_t<!C, MatPursuer<N + 1, Qualifier>> operator[](uint32_t i)
		{
			assert(i >= 0 && i < mMatrix.GetRange()[N]);
			return MatPursuer<N + 1, Qualifier>(mMatrix, mRow * mMatrix.GetRange()[N] + i);
		}

		MatPursuer<N + 1, Qualifier> operator[](uint32_t i) const
		{
			assert(i >= 0 && i < mMatrix.GetRange()[N]);
			return MatPursuer<N + 1, Qualifier>(mMatrix, mRow * mMatrix.GetRange()[N] + i);
		}
	private:
		std::size_t mRow;
		Qualifier<Matrix<T, Dim>>& mMatrix;
	};
	template <int N, template <class> class Qualifier>
	class MatPursuer<N, Qualifier, true>
	{
	public:
		MatPursuer(Qualifier<Matrix<T, Dim>>& m, std::size_t row) : mRow(row), mMatrix(m) {}

		template <bool C = std::is_const<MatPursuer<N - 1, Qualifier>>::value>
		std::enable_if_t<!C, T&> operator[](uint32_t i)
		{
			assert(i >= 0 && i < mMatrix.GetRange()[N]);
			return mMatrix.mMatrixData[mRow * mMatrix.GetRange()[N] + i];
		}

		const T& operator[](uint32_t i) const
		{
			assert(i >= 0 && i < mMatrix.GetRange()[N]);
			return mMatrix.mMatrixData[mRow * mMatrix.GetRange()[N] + i];
		}
	private:
		std::size_t mRow;
		Qualifier<Matrix<T, Dim>>& mMatrix;
	};

	//Dim>1のときはMatPursuerを返す。
	template <bool B = (Dim > 1)>
	std::enable_if_t<B, MatPursuer<1, IdentityT>> operator[](uint32_t i)
	{
		assert(i >= 0 && i < GetRange()[0]);
		return MatPursuer<1, IdentityT>(*this, i);
	}
	template <bool B = (Dim > 1)>
	std::enable_if_t<B, MatPursuer<1, std::add_const_t>> operator[](uint32_t i) const
	{
		assert(i >= 0 && i < GetRange()[0]);
		return MatPursuer<1, std::add_const_t>(*this, i);
	}

	//Dim == 1のときはMatPursuerを介す必要はない。
	template <bool B = (Dim == 1)>
	std::enable_if_t<B, T&> operator[](uint32_t i)
	{
		assert(i >= 0 && i < GetRange()[0]);
		return mMatrixData[i];
	}
	template <bool B = (Dim == 1)>
	std::enable_if_t<B, const T&> operator[](uint32_t i) const
	{
		assert(i >= 0 && i < GetRange()[0]);
		return mMatrixData[i];
	}

	bool operator==(const Matrix& m) const
	{
		if (GetRange() != m.GetRange()) return false;
		for (auto t : BundleRange(*this, m)) if (std::get<0>(t) != std::get<1>(t)) return false;
		return true;
	}
	bool operator!=(const Matrix& m) const
	{
		return operator==(m);
	}


	//[0][0], [0][1], [0][2], ...みたいな順に走査する。はず。
	T* begin() { return mMatrixData; }
	const T* begin() const { return mMatrixData; }
	const T* cbegin() const { return begin(); }
	T* end() { return mMatrixData + std::accumulate(GetRange().begin(), GetRange().end(), 1, std::multiplies<uint32_t>()); }
	const T* end() const { return mMatrixData + std::accumulate(GetRange().begin(), GetRange().end(), 1, std::multiplies<uint32_t>()); }
	const T* cend() const { return end(); }

	Matrix& operator*=(const T& x) { for (auto& e : *this) e *= x; return *this; }
	Matrix& operator/=(const T& x) { for (auto& e : *this) e /= x; return *this; }

	friend Matrix operator*(const Matrix& m, const T& t) { Matrix res = m; res *= t; return std::move(res); }
	friend Matrix operator*(const T& t, const Matrix& m) { Matrix res = m; res *= t; return std::move(res); }
	friend Matrix operator/(const Matrix& m, const T& t) { Matrix res = m; res /= t; return std::move(res); }
	friend Matrix operator/(const T& t, const Matrix& m) { Matrix res = m; res /= t; return std::move(res); }

	Matrix& operator+=(const Matrix& x)
	{
		assert(GetRange() == x.GetRange());
		for (const auto& t : BundleRange(*this, x))
			std::get<0>(t) += std::get<1>(t);
		return *this;
	}
	friend Matrix operator+(const Matrix& a, const Matrix& b)
	{
		Matrix res(a);
		res += b;
		return std::move(res);
	}
	Matrix& operator-=(const Matrix& x)
	{
		assert(GetRange() == x.GetRange());
		for (const auto& t : BundleRange(*this, x))
			std::get<0>(t) -= std::get<1>(t);
		return *this;
	}
	friend Matrix operator-(const Matrix& a, const Matrix& b)
	{
		Matrix res(a);
		res -= b;
		return std::move(res);
	}

	//配列の再確保のための関数。
	//mMatrixDataがnullptrであることを前提としている。
	template <bool B = (Dim == 1), std::enable_if_t<B, std::nullptr_t> = nullptr>
	void Resize(uint32_t x)
	{
		size_t cap = (size_t)x;
		if (!mMatrixData) Allocate(cap);
		else if (GetCapacity() != cap) Reallocate(cap);
		GetRange() = { x };
		ConstructAll(cap, T());
	}
	template <bool B = (Dim == 2), std::enable_if_t<B, std::nullptr_t> = nullptr>
	void Resize(uint32_t x, uint32_t y)
	{
		size_t cap = (size_t)x * (size_t)y;
		if (!mMatrixData) Allocate(cap);
		else if (GetCapacity() != cap) Reallocate(cap);
		GetRange() = { x, y };
		ConstructAll(cap, T());
	}
	template <bool B = (Dim == 3), std::enable_if_t<B, std::nullptr_t> = nullptr>
	void Resize(uint32_t x, uint32_t y, uint32_t z)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z;
		if (!mMatrixData) Allocate(cap);
		else if (GetCapacity() != cap) Reallocate(cap);
		GetRange() = { x, y, z };
		ConstructAll(cap, T());
	}
	template <bool B = (Dim == 4), std::enable_if_t<B, std::nullptr_t> = nullptr>
	void Resize(uint32_t x, uint32_t y, uint32_t z, uint32_t t)
	{
		size_t cap = (size_t)x * (size_t)y * (size_t)z * (size_t)t;
		if (!mMatrixData) Allocate(cap);
		else if (GetCapacity() != cap) Reallocate(cap);
		GetRange() = { x, y, z, t };
		ConstructAll(cap, T());
	}

protected:

	void Allocate(size_t size)
	{
		assert(mMatrixData == nullptr);
		char* p = (char*)malloc(size * sizeof(T) + msRangeSize);
		mMatrixData = (T*)(p + msRangeSize);
	}
	void Reallocate(size_t size)
	{
		Destroy();
		Allocate(size);
	}
	void Destroy()
	{
		if (mMatrixData == nullptr) return;
		DestructAll(GetCapacity());
		char* ptr = (char*)mMatrixData;
		free(ptr - msRangeSize);
		mMatrixData = nullptr;
	}
	void ConstructAll(size_t cap, const T& t)
	{
		T* it = mMatrixData;
		T* end = mMatrixData + cap;
		for (; it != end; ++it) new (it) T(t);
	}
	void DestructAll(size_t cap)
	{
		//メモリ解放を行わない、ただ要素を全削除するもの。
		T* it = mMatrixData;
		T* end = mMatrixData + cap;
		for (; it != end; ++it) it->~T();
	}

	Range& GetRange() { return *reinterpret_cast<Range*>((char*)mMatrixData - msRangeSize); }
	const Range& GetRange() const { return *reinterpret_cast<const Range*>((char*)mMatrixData - msRangeSize); }

	//std::array<uint32_t, Dim> mSize;
	T* mMatrixData;
};

template <class T>
using Vector = Matrix<T, 1>;

template <class T>
Matrix<T, 2> MakeMatrix(const Vector<T>& r1, const Vector<T>& r2)
{
	Matrix<T, 2> res;
	MakeMatrix(res, r1, r2);
	return std::move(res);
}
template <class T>
void MakeMatrix(Matrix<T, 2>& res, const Vector<T>& r1, const Vector<T>& r2)
{
	uint32_t sc = r1.GetSize(0);
	assert(sc == r2.GetSize(0));
	res.Resize(2, sc);
	for (uint32_t c = 0; c < sc; ++c) res[0][c] = r1[c];
	for (uint32_t c = 0; c < sc; ++c) res[1][c] = r2[c];
}
template <class T>
Matrix<T, 2> MakeMatrix(const Vector<T>& r1, const Vector<T>& r2, const Vector<T>& r3)
{
	Matrix<T, 2> res;
	MakeMatrix(res, r1, r2, r3);
	return std::move(res);
}
template <class T>
void MakeMatrix(Matrix<T, 2>& res, const Vector<T>& r1, const Vector<T>& r2, const Vector<T>& r3)
{
	uint32_t sc = r1.GetSize(0);
	assert(sc == r2.GetSize(0));
	assert(sc == r3.GetSize(0));
	res.Resize(3, sc);
	for (uint32_t c = 0; c < sc; ++c) res[0][c] = r1[c];
	for (uint32_t c = 0; c < sc; ++c) res[1][c] = r2[c];
	for (uint32_t c = 0; c < sc; ++c) res[2][c] = r3[c];
}

template <class T>
Matrix<T, 2> operator*(const Matrix<T, 2>& x, const Matrix<T, 2>& y)
{
	Matrix<T, 2> res;
	Multiply(res, x, y);
	return std::move(res);
}
template <class T>
void Multiply(Matrix<T, 2>& res, const Matrix<T, 2>& x, const Matrix<T, 2>& y)
{
	assert(x.GetSize(1) == y.GetSize(0));
	uint32_t s = x.GetSize(1);
	uint32_t sx = x.GetSize(0);
	uint32_t sy = y.GetSize(1);
	res.Resize(sx, sy);
	for (uint32_t i = 0; i < sx; ++i)
	{
		for (uint32_t j = 0; j < sy; ++j)
		{
			T v = 0;
			for (uint32_t k = 0; k < s; ++k)
			{
				v += x[i][k] * y[k][j];
			}
			res[i][j] = v;
		}
	}
}
template <class T>
Vector<T> operator*(const Matrix<T, 2>& x, const Vector<T>& y)
{
	Vector<T> res;
	Multiply(res, x, y);
	return std::move(res);
}
template <class T>
void Multiply(Vector<T>& res, const Matrix<T, 2>& x, const Vector<T>& y)
{
	uint32_t sx = x.GetSize(0);
	uint32_t sy = x.GetSize(1);
	assert(sx == y.GetSize(0));
	res.Resize(sx);
	for (uint32_t i = 0; i < sx; ++i)
	{
		T v = 0;
		for (uint32_t j = 0; j < sy; ++j)
		{
			v += x[i][j] * y[j];
		}
		res[i] = v;
	}
}

template <class T>
Matrix<T, 2> Transpose(const Matrix<T, 2>& x)
{
	Matrix<T, 2> res;
	Transpose(res, x);
	return std::move(res);
}
template <class T>
void Transpose(Matrix<T, 2>& res, const Matrix<T, 2>& x)
{
	uint32_t sx = x.GetSize(0), sy = x.GetSize(1);
	res.Resize(sx, sy);
	for (uint32_t i = 0; i < sx; ++i)
	{
		for (uint32_t j = 0; j < sy; ++j)
		{
			res[j][i] = x[i][j];
		}
	}
}

//内積
template <class T>
T Dot(const Vector<T>& x, const Vector<T>& y)
{
	assert(x.GetSize(0) == y.GetSize(0));
	T res = 0;
	for (auto t : BundleRange(x, y)) res += std::get<0>(t) * std::get<1>(t);
	return std::move(res);
}
//外積
template <class T>
Vector<T> Cross(const Vector<T>& a, const Vector<T>& b)
{
	assert(a.GetSize(0) == 3 && b.GetSize(0) == 3);
	Vector<T> res;
	Cross(res, a, b);
	return std::move(res);
}
template <class T>
void Cross(Vector<T>& res, const Vector<T>& a, const Vector<T>& b)
{
	assert(a.GetSize(0) == 3 && b.GetSize(0) == 3);
	res.Resize(a.GetSize(0));
	double ax = a[0], ay = a[1], az = a[2];
	double bx = b[0], by = b[1], bz = b[2];
	res[0] = ay * bz - az * by;
	res[1] = az * bx - ax * bz;
	res[2] = ax * by - ay * bx;
}

}

}

#endif