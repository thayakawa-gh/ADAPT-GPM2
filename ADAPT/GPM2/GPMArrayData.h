#ifndef GPM2_GPMARRAYDATA_H
#define GPM2_GPMARRAYDATA_H

#include <vector>
#include <string>
#ifdef ADAPT_USE_CPP20
#include <ranges>
#define GPM2_NS_STD_RANGE std::ranges::
#else
#define GPM2_NS_STD_RANGE std::
#endif
#include <ADAPT/CUF/Matrix.h>
#include <ADAPT/CUF/Variant.h>
#include <ADAPT/CUF/Generics.h>
#include <ADAPT/GPM2/GPMCore.h>

namespace adapt
{

namespace gpm2
{

namespace detail
{

struct ToFPtr
{
	template <class ...Args>
	void operator()(Args&& ...args) const
	{
		adapt::Print(pipe, std::forward<Args>(args)...);
	}
	FILE* pipe;
};
struct ToOStr
{
	template <class ...Args>
	void operator()(Args&& ...args)
	{
		adapt::Print(ofs, std::forward<Args>(args)...);
	}
	std::ofstream& ofs;
};

struct ArrayData
{
	enum Type { NUM_RANGE, STR_RANGE, COLUMN, UNIQUE, };
private:
	template <class T>
	struct IsRange_impl
	{
		template <class U>
		static constexpr auto check(const U& u, int)
			-> decltype(GPM2_NS_STD_RANGE begin(u), GPM2_NS_STD_RANGE end(u), std::true_type());
		template <class U>
		static constexpr auto check(const U& u, long)
			->std::false_type;
		static constexpr bool value = decltype(check(std::declval<T>(), 1))::value;
	};
public:
	template <class T>
	static constexpr bool IsRange() { return IsRange_impl<T>::value; }
	template <class T>
	static constexpr bool IsNumericRange()
	{
		if constexpr (!IsRange<T>()) return false;
		//char[N]のような固定長配列やstd::stringはarithmeticかつrangeであるが、
		//IsColumnと識別するために、
		//std::string_viewに変換可能なものは弾く。
		else if constexpr (std::is_convertible_v<T, std::string_view>) return false;
		else
		{
			using ValueType = RemoveCVRefT<decltype(*GPM2_NS_STD_RANGE begin(std::declval<T&>()))>;
			if constexpr (std::is_arithmetic_v<ValueType>) return true;
			else return false;
		}
	}
	template <class T>
	static constexpr bool IsStringRange()
	{
		if constexpr (!IsRange<T>()) return false;
		else
		{
			using ValueType = decltype(*GPM2_NS_STD_RANGE begin(std::declval<T&>()));
			if constexpr (std::is_convertible_v<ValueType, std::string_view>) return true;
			else return false;
		}
	}
	template <class T>
	static constexpr bool IsColumn()
	{
		return std::is_convertible_v<T, std::string_view>;
	}
	template <class T>
	static constexpr bool IsUnique()
	{
		return std::is_arithmetic_v<T>;
	}

private:
	struct Increment_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = void;
		template <class T, class S>
		void operator()(T& it, const S&) const
		{
			//Genericsにstd::anyを使う場合、Tがconstとnon constの両方の場合がコンパイル対象になる。
			//可能なら呼び出し不可能な方は自動的に例外でも投げるべきなんだが、難しい。
			if constexpr (std::is_const_v<T>) throw InvalidArg("");
			else ++it;
		}
	};
	struct Indirection_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = double;
		template <class T, class S>
		double operator()(const T& it, const S&) const
		{
			if constexpr (std::is_convertible_v<decltype(*it), double>) return (double)*it;
			else throw InvalidType("");
		}
	};
	struct Compare_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = bool;
		template <class T, class S>
		bool operator()(const T& it, const S& last) const
		{
			return it == last;
		}
	};
	template <class Func>
	struct OutFunc
	{
		using ArgTypes = std::tuple<Func>;
		using RetType = void;
		template <class T, class S>
		void operator()(Func output_func, const T& it, const S&) const
		{
			output_func(" ", *it, print::end<'\0'>());
		}
	};

public:
	struct Range
	{
		template <class A, class B>
		Range(A&& a, B&& b) : mRange(std::forward<A>(a), std::forward<B>(b)) {}
		void operator++() { mRange.Visit<0>(); }
		double operator*() const { return mRange.Visit<1>(); }
		bool End() const { return mRange.Visit<2>(); }
		void Output(ToFPtr o) const { mRange.Visit<3>(o); }
		void Output(ToOStr o) const { mRange.Visit<4>(o); }
	private:
		Generics<std::tuple<Any, Any>,
			std::tuple<Increment_impl, Indirection_impl, Compare_impl, OutFunc<ToFPtr>, OutFunc<ToOStr>>>
			mRange;
	};
private:
	struct GetType_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = Type;
		template <class T>
		Type operator()(const T&) const
		{
			if constexpr (IsNumericRange<T>()) return NUM_RANGE;
			else if constexpr (IsStringRange<T>()) return STR_RANGE;
			else if constexpr (IsColumn<T>()) return COLUMN;
			else if constexpr (IsUnique<T>()) return UNIQUE;
			else throw InvalidType("no value is contained");
		}
	};
	struct Range_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = Range;
		template <class T>
		Range operator()([[maybe_unused]] const T& v) const
		{
			if constexpr (!IsNumericRange<T>() && !IsStringRange<T>()) throw InvalidType("contained value is not a range");
			else return Range(GPM2_NS_STD_RANGE begin(v), GPM2_NS_STD_RANGE end(v));
		}
	};
	struct GetColumn_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = std::string_view;
		template <class T>
		std::string_view operator()([[maybe_unused]] T t) const
		{
			if constexpr (IsColumn<T>()) return t;
			else throw InvalidType("contained value is not a std::string");
		}
	};
	struct GetUnique_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = double;
		template <class T>
		double operator()([[maybe_unused]] const T& d) const
		{
			if constexpr (IsUnique<T>()) return d;
			else throw InvalidType("contained value is not a double");
		}
	};

	using GenCRef = Generics<AnyCRef, std::tuple<GetType_impl, Range_impl, GetColumn_impl, GetUnique_impl>>;

public:

	struct RangeAdapter
	{
		template <class T, std::enable_if_t<IsNumericRange<T>() || IsStringRange<T>(), std::nullptr_t> = nullptr>
		RangeAdapter(const T& v) : mRange(v) {}
		GenCRef mRange;
	};

	ArrayData() : mAnyCRef(Any::NullType{}) {}
	template <class T, std::enable_if_t<IsNumericRange<T>(), std::nullptr_t> = nullptr>
	ArrayData(const T& nr) : mAnyCRef(nr) {}
	template <class T, std::enable_if_t<IsStringRange<T>(), std::nullptr_t> = nullptr>
	ArrayData(const T& sr) : mAnyCRef(sr) {}
	template <class T, std::enable_if_t<IsColumn<T>(), std::nullptr_t> = nullptr>
	ArrayData(const T& c) : mAnyCRef(c) {}
	template <class T, std::enable_if_t<IsUnique<T>(), std::nullptr_t> = nullptr>
	ArrayData(const T& u) : mAnyCRef(u) {}
	ArrayData(RangeAdapter r) : mAnyCRef(std::move(r.mRange)) {}
	Type GetType() const { return mAnyCRef.Visit<0>(); }
	Range GetRange() const { return mAnyCRef.Visit<1>(); }
	std::string_view GetColumn() const { return mAnyCRef.Visit<2>(); }
	double GetUnique() const { return mAnyCRef.Visit<3>(); }

	bool IsEmpty() const { return mAnyCRef.GetAny<0>().IsEmpty(); }
	operator bool() const { return !mAnyCRef.GetAny<0>().IsEmpty(); }

private:
	GenCRef mAnyCRef;
};

using RangeAdapter = ArrayData::RangeAdapter;

struct MatrixData
{
	enum Type { NUM_MAT, COLUMN, UNIQUE, };

private:

	template <class X> using Mat = adapt::Matrix<X>;
	template <class T>
	static constexpr bool IsNumericMatrix()
	{
		return IsBasedOn_T<T, Mat>::value;
	}
	template <class T>
	static constexpr bool IsColumn()
	{
		return std::is_convertible_v<T, std::string_view>;
	}
	template <class T>
	static constexpr bool IsUnique()
	{
		return std::is_arithmetic_v<T>;
	}

	struct GetType_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = Type;
		template <class T>
		Type operator()(const T&) const
		{
			if constexpr (IsNumericMatrix<T>()) return NUM_MAT;
			else if constexpr (IsColumn<T>()) return COLUMN;
			else if constexpr (IsUnique<T>()) return UNIQUE;
			else throw InvalidType("no value is contained");
		}
	};

	struct GetSize_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = std::pair<uint32_t, uint32_t>;
		template <class T>
		std::pair<uint32_t, uint32_t> operator()(const T& t) const
		{
			return { t.GetSize(0), t.GetSize(1) };
		}
	};
	template <class Func>
	struct Output_impl
	{
		using ArgTypes = std::tuple<Func, double, double, double, double, uint32_t, uint32_t>;
		using RetType = void;
		template <class T>
		void operator()(Func output_func,
						double x, double y, double cx, double cy,
						uint32_t ix, uint32_t iy, const T& map) const
		{
			output_func(x, y, cx, cy, map[ix][iy]);
		}
	};
public:
	struct MatRef
	{
		template <class T>
		MatRef(const Matrix<T>& m) : mMat(m) {}
		std::pair<uint32_t, uint32_t> GetSize() const { return mMat.Visit<0>(); }
		void Out(ToFPtr f, double x, double y, double cx, double cy, uint32_t ix, uint32_t iy) const
		{
			mMat.Visit<1>(f, x, y, cx, cy, ix, iy);
		}
		void Out(ToOStr f, double x, double y, double cx, double cy, uint32_t ix, uint32_t iy) const
		{
			mMat.Visit<2>(f, x, y, cx, cy, ix, iy);
		}
	private:
		Generics<AnyCRef, std::tuple<GetSize_impl, Output_impl<ToFPtr>, Output_impl<ToOStr>>> mMat;
	};
private:
	struct GetNumMat_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = MatRef;
		template <class T>
		MatRef operator()([[maybe_unused]] const T& t) const
		{
			if constexpr (IsNumericMatrix<T>()) return t;
			else throw InvalidType("contained value is not a adapt::Matrix");
		}
	};
	struct GetColumn_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = std::string_view;
		template <class T>
		std::string_view operator()([[maybe_unused]] T t) const
		{
			if constexpr (IsColumn<T>()) return t;
			else throw InvalidType("contained value is not a std::string");
		}
	};
	struct GetUnique_impl
	{
		using ArgTypes = std::tuple<>;
		using RetType = double;
		template <class T>
		double operator()([[maybe_unused]] const T& d) const
		{
			if constexpr (IsUnique<T>()) return d;
			else throw InvalidType("contained value is not a double");
		}
	};

	using CRef = Generics<AnyCRef, std::tuple<GetType_impl, GetNumMat_impl, GetColumn_impl, GetUnique_impl>>;

public:

	MatrixData() : mAnyCRef(Any::NullType{}) {}
	template <class T>
	MatrixData(const Matrix<T>& matrix) : mAnyCRef(matrix) {}
	template <class T, std::enable_if_t<IsColumn<T>(), std::nullptr_t> = nullptr>
	MatrixData(const T& c) : mAnyCRef(c) {}
	template <class T, std::enable_if_t<IsUnique<T>(), std::nullptr_t> = nullptr>
	MatrixData(const T& u) : mAnyCRef(u) {}

	Type GetType() const { return mAnyCRef.Visit<0>(); }
	MatRef GetMatrix() const { return mAnyCRef.Visit<1>(); }
	std::string_view GetColumn() const { return mAnyCRef.Visit<2>(); }
	double GetUnique() const { return mAnyCRef.Visit<3>(); }

	bool IsEmpty() const { return mAnyCRef.GetAny<0>().IsEmpty(); }
	operator bool() const { return !IsEmpty(); }

private:

	CRef mAnyCRef;
};

}

}

}

#endif