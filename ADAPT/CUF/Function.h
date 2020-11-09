#ifndef CUF_FUNCTION_H
#define CUF_FUNCTION_H

#include <string>
#include <vector>
#include <array>
#include <mutex>
#include <random>
#include <sstream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <ADAPT/CUF/Template.h>

namespace adapt
{

inline namespace cuf
{

//string中のfrom文字をto文字へと置き換える関数。
inline std::string ReplaceStr(const std::string& str, const std::string& from, const std::string& to)
{
	std::string res = str;
	std::string::size_type pos = 0;
	while (pos = res.find(from, pos), pos != std::string::npos)
	{
		res.replace(pos, from.length(), to);
		pos += to.length();
	}
	return res;
}

inline std::vector<std::string> SplitStr(const std::string& str)
{
	std::vector<std::string> res;
	std::istringstream ss(str);
	std::string buffer;
	while (ss >> buffer) res.push_back(buffer);
	return res;
}
inline std::vector<std::string> SplitStr(const std::string& str, char delim)
{
	std::vector<std::string> res;
	std::stringstream ss(str);
	std::string buffer;
	while (std::getline(ss, buffer, delim))
	{
		res.push_back(buffer);
	}
	return res;
}

inline void DelSpace(std::string &str)
{
	std::size_t i;
	while ((i = str.find_first_of(" ")) != std::string::npos)
	{
		str.erase(i, 1);
	}
}
inline unsigned int GetSeed()
{
	static std::mutex m;
	static std::random_device rd;
	std::lock_guard<std::mutex> lg(m);
	return rd();
}
template <class Type>
std::vector<Type> UniqueRandom(Type min, Type max, Type num)
{
	static std::random_device rd;
	static std::mt19937_64 mt(rd());
	std::vector<Type> r(max - min + 1);
	std::iota(r.begin(), r.end(), min);
	std::shuffle(r.begin(), r.end(), mt);
	r.erase(r.begin() + num, r.end());
	return r;
}

template <class T, std::enable_if_t<std::is_arithmetic<T>::value, std::nullptr_t> = nullptr>
std::string ToHexString(T value)
{
	std::ostringstream os;
	if (value < 0) os << '-', value *= -1;
	os << std::hex << value;
	return os.str();
}
template <class T, std::enable_if_t<std::is_arithmetic<T>::value, std::nullptr_t> = nullptr>
std::string ToOctString(T value)
{
	std::ostringstream os;
	if (value < 0) os << '-', value *= -1;
	os << std::oct << value;
	return os.str();
}
template <class T, std::enable_if_t<std::is_arithmetic<T>::value, std::nullptr_t> = nullptr>
std::string ToString(T value)
{
	std::ostringstream os;
	os << std::dec << value;
	return os.str();
}
template <class T, std::enable_if_t<std::is_convertible<T, std::string>::value, std::nullptr_t> = nullptr>
std::string ToString(const T& c)
{
	return std::string(c);
}

namespace detail
{

template <class T>
struct GetArraySize_impl;
template <class Type, size_t N>
struct GetArraySize_impl<std::array<Type, N>> { static constexpr size_t Size = N; };

template <class Array>
constexpr size_t GetArraySize() { return GetArraySize_impl<std::decay_t<Array>>::Size; }

template <bool Rval>
struct Forward_impl
{
	template <class Type>
	static constexpr std::remove_reference_t<Type>& apply(Type&& t) { return static_cast<std::remove_reference_t<Type>&>(t); }
};
template <>
struct Forward_impl<true>
{
	template <class Type>
	static constexpr std::remove_reference_t<Type>&& apply(Type&& t) { return static_cast<std::remove_reference_t<Type>&&>(t); }
};

template <bool Rval, class Type>
constexpr decltype(auto) Forward(Type&& v) { return Forward_impl<Rval>::apply(std::forward<Type>(v)); }

template <class Array1, size_t ...Indices1, class Array2, size_t ...Indices2>
constexpr auto CatArray_impl(Array1&& a, std::index_sequence<Indices1...>,
							 Array2&& b, std::index_sequence<Indices2...>)
{
	constexpr size_t N1 = detail::GetArraySize<Array1>();
	constexpr size_t N2 = detail::GetArraySize<Array2>();
	using Type = typename std::decay_t<Array1>::value_type;
	static_assert(std::is_same<Type, typename std::decay_t<Array2>::value_type>::value, "");
	constexpr bool L1 = std::is_lvalue_reference<Array1>::value;
	constexpr bool L2 = std::is_lvalue_reference<Array2>::value;
#if __cplusplus >= 201703L
	return std::array<Type, N1 + N2>{ Forward<!L1>(a[Indices1])..., Forward<!L2>(b[Indices2])... };
#else if __cplusplus >= 201402L
	return std::array<Type, N1 + N2>{ static_cast<const std::remove_reference_t<Array1>&>(a)[Indices1]...,
									  static_cast<const std::remove_reference_t<Array2>&>(b)[Indices2]... };
#endif
}
}

template <class Array>
constexpr Array CatArray(Array&& a)
{
	return a;
}
template <class Array1, class Array2>
constexpr auto CatArray(Array1&& a, Array2&& b)
{
	constexpr size_t N1 = detail::GetArraySize<Array1>();
	constexpr size_t N2 = detail::GetArraySize<Array2>();
	return detail::CatArray_impl(std::forward<Array1>(a), std::make_index_sequence<N1>(),
								 std::forward<Array2>(b), std::make_index_sequence<N2>());
}

namespace detail
{
template <class Array1, class Array2>
constexpr auto CatArray_rec(Array1&& a, Array2&& b)
{
	return CatArray(std::forward<Array1>(a), std::forward<Array2>(b));
}
template <class Array1, class Array2, class Array3, class ...Arrays>
constexpr auto CatArray_rec(Array1&& a, Array2&& b, Array3&& c, Arrays&& ...as)
{
	return CatArray_rec(CatArray(std::forward<Array1>(a), std::forward<Array2>(b)), std::forward<Array3>(c), std::forward<Arrays>(as)...);
}
}
template <class ...Array>
constexpr auto CatArray(Array&& ...a)
{
	return detail::CatArray_rec(std::forward<Array>(a)...);
}
template <class ...Args>
constexpr auto MakeArray(Args&& ...args)->
std::array<
	typename std::decay_t<
	typename std::common_type_t<Args...>>,
	sizeof...(Args)>
{
	return std::array<
		typename std::decay<
		typename std::common_type<Args...>::type>::type,
		sizeof...(Args)>{ std::forward<Args>(args)... };
}

namespace detail
{
template <class Func, class T>
constexpr auto Accumulate_impl(Func f, T&& a)
{
	return a;
}
template <class Func, class T, class U, class ...V>
constexpr auto Accumulate_impl(Func f, T&& a, U&& b, V&& ...v)
{
	return Accumulate_impl(f, f(std::forward<T>(a), std::forward<U>(b)), std::forward<V>(v)...);
}

//C++標準のstd::plusなどは左右の引数型が一致しないと使えないクソ仕様なので、
//こちらで用意する。
struct AccPlus
{
	template <class T, class U>
	constexpr auto operator()(T&& t, U&& u) const { return std::forward<T>(t) + std::forward<U>(u); }
};
struct AccTimes
{
	template <class T, class U>
	constexpr auto operator()(T&& t, U&& u) const { return std::forward<T>(t) * std::forward<U>(u); }
};
struct AccAnd
{
	template <class T, class U>
	constexpr auto operator()(T&& t, U&& u) const { return std::forward<T>(t) && std::forward<U>(u); }
};
struct AccOr
{
	template <class T, class U>
	constexpr auto operator()(T&& t, U&& u) const { return std::forward<T>(t) || std::forward<U>(u); }
};
}

template <class ...T>
constexpr auto Summation(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccPlus(), std::forward<T>(v)...);
}
template <class ...T>
constexpr auto ProductNotation(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccTimes(), std::forward<T>(v)...);
}
template <class ...T>
constexpr auto AndAll(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccAnd(), std::forward<T>(v)...);
}
template <class ...T>
constexpr auto OrAll(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccOr(), std::forward<T>(v)...);
}

namespace detail
{
template <class T1, class T2, class ...T>
constexpr auto EqualAll_impl(T1&& v1, T2&& v2, T&& ...v)
{
	return std::forward<T1>(v1) == std::forward<T2>(v2) &&
		EqualAll_impl(std::forward<T2>(v2), std::forward<T>(v)...);
}
template <class T>
constexpr auto EqualAll_impl(T&& v) { return true; }

}
//任意数の引数について、隣り合う二つをすべての組み合わせ（sizeof...(T) - 1通り）で比較し、
//いずれの組も値が一致するのならtrueを、そうでなければfalseを返す。
template <class ...T, std::enable_if_t<(sizeof...(T) > 1), std::nullptr_t> = nullptr>
constexpr auto EqualAll(T&& ...v)
{
	detail::EqualAll_impl(std::forward<T>(v)...);
}

#ifndef _MSC_VER
	inline int _fseeki64(FILE* stream, int64_t offset, int origin)\
{\
return fseek(stream, offset, origin); \
}
	inline int64_t _ftelli64(FILE* stream)
	{
		\
		return ftell(stream); \
}
#endif

#ifndef _MSC_VER
inline FILE* _popen(const char* path, const char* mode)\
{\
return popen(path, mode); \
}
inline int _pclose(FILE* stream)\
{\
return pclose(stream); \
}
#endif


namespace detail
{

template <class Iterators, class IndexSequence>
class BundledIterator_impl;
template <class ...Iterators, std::size_t ...Indices>
class BundledIterator_impl<std::tuple<Iterators...>, std::index_sequence<Indices...>>
{
public:
	BundledIterator_impl(Iterators... cs)
		: mIterators(cs...)
	{}

	BundledIterator_impl& operator++()
	{
		int d[] = { (++std::get<Indices>(mIterators), 0)... };
		return *this;
	}
	auto operator*() const noexcept
	{
		return std::forward_as_tuple(*std::get<Indices>(mIterators)...);
	}
	bool operator==(const BundledIterator_impl& it) const
	{
		return std::get<0>(mIterators) == std::get<0>(it.mIterators);
	}
	bool operator!=(const BundledIterator_impl& it) const
	{
		return !(*this == it);
	}

private:
	std::tuple<Iterators...> mIterators;

};

}

template <class ...Iterators>
using BundledIterator = detail::BundledIterator_impl<std::tuple<Iterators...>, std::make_index_sequence<sizeof...(Iterators)>>;

template <class ...Iterators>
class BundledIteratorWithIndex
	: public BundledIterator<Iterators...>
{
public:
	BundledIteratorWithIndex(Iterators... cs) : BundledIterator<Iterators...>(cs...), mIndex(0) {}

	BundledIteratorWithIndex& operator++()
	{
		++mIndex;
		BundledIterator<Iterators...>::operator++();
		return *this;
	}
	auto operator*() const noexcept
	{
		return std::tuple_cat(std::make_tuple(mIndex), BundledIterator<Iterators...>::operator*());
	}

private:
	std::size_t mIndex;
};


namespace detail
{

template <class ...Iterators>
BundledIterator<Iterators...> MakeBundledIterator(Iterators ...it)
{
	return BundledIterator<Iterators...>(it...);
}
template <class ...Iterators>
BundledIteratorWithIndex<Iterators...> MakeBundledIteratorWithIndex(Iterators ...it)
{
	return BundledIteratorWithIndex<Iterators...>(it...);
}


template <class Iterators, class IndexSequence>
class BundledRange_impl;
template <class ...Containers, std::size_t ...Indices>
class BundledRange_impl<std::tuple<Containers...>, std::index_sequence<Indices...>>
{
public:

	template <class ...C>
	BundledRange_impl(C&&... c)
		: mContainers(std::forward<C>(c)...) {}

	auto begin() const { return MakeBundledIterator(std::get<Indices>(mContainers).begin()...); }
	auto begin() { return MakeBundledIterator(std::get<Indices>(mContainers).begin()...); }
	auto end() const { return MakeBundledIterator(std::get<Indices>(mContainers).end()...); }
	auto end() { return MakeBundledIterator(std::get<Indices>(mContainers).end()...); }

private:
	std::tuple<Containers...> mContainers;
};

}

template <class ...Containers>
using BundledRange = detail::BundledRange_impl<std::tuple<Containers...>, std::make_index_sequence<sizeof...(Containers)>>;

namespace detail
{

template <class Iterators, class IndexSequence>
class BundledRangeWithIndex_impl;
template <class ...Containers, std::size_t ...Indices>
class BundledRangeWithIndex_impl<std::tuple<Containers...>, std::index_sequence<Indices...>>
{
public:

	template <class ...C>
	BundledRangeWithIndex_impl(C&&... c)
		: mContainers(std::forward<C>(c)...) {}

	auto begin() const
	{
		return MakeBundledIteratorWithIndex(std::get<Indices>(mContainers).begin()...);
	}
	auto end() const
	{
		return MakeBundledIteratorWithIndex(std::get<Indices>(mContainers).end()...);
	}

private:
	std::tuple<Containers...> mContainers;
};

}

template <class ...Containers>
using BundledRangeWithIndex = detail::BundledRangeWithIndex_impl<std::tuple<Containers...>, std::make_index_sequence<sizeof...(Containers)>>;

template <class ...Containers>
BundledRange<Containers...> BundleRange(Containers&& ...cs)
{
	return BundledRange<Containers...>(std::forward<Containers>(cs)...);
}

template <class ...Containers>
BundledRangeWithIndex<Containers&&...> BundleRangeWithIndex(Containers&& ...cs)
{
	return BundledRangeWithIndex<Containers...>(std::forward<Containers>(cs)...);
}

namespace detail
{

template <class T, std::size_t N>
class ReferenceArray
{
public:

	template <class ...Args>
	ReferenceArray(Args&& ...args) : mPtrArray{ &std::forward<Args>(args)... } {}

	class Iterator
	{
	public:
		Iterator(typename std::array<std::add_pointer_t<T>, N>::iterator i) : mIterator(i) {}

		Iterator& operator++()
		{
			++mIterator;
			return *this;
		}
		T& operator*() const noexcept { return *(*mIterator); }
		bool operator==(const Iterator& it2) const { return mIterator == it2.mIterator; }
		bool operator!=(const Iterator& it2) const { return !(*this == it2); }

	private:
		typename std::array<std::add_pointer_t<T>, N>::iterator mIterator;
	};
	class ConstIterator
	{
	public:
		ConstIterator(typename std::array<std::add_pointer_t<T>, N>::const_iterator i) : mIterator(i) {}

		ConstIterator& operator++()
		{
			++mIterator;
			return *this;
		}
		const T& operator*() const noexcept { return *(*mIterator); }
		bool operator==(const Iterator& it2) const { return mIterator == it2.mIterator; }
		bool operator!=(const Iterator& it2) const { return !(*this == it2); }

	private:
		typename std::array<std::add_pointer_t<T>, N>::const_iterator mIterator;
	};

	constexpr std::size_t size() const { return N; }
	Iterator begin() { return Iterator(mPtrArray.begin()); }
	//ConstIterator begin() const { return ConstIterator(mPtrArray.begin()); }
	Iterator end() { return Iterator(mPtrArray.end()); }
	//ConstIterator end() const { return ConstIterator(mPtrArray.end()); }

private:

	std::array<std::add_pointer_t<T>, N> mPtrArray;
};

}

template <class ...Args>
detail::ReferenceArray<typename CommonType<Args...>::Type, sizeof...(Args)> HoldRefArray(Args& ...args)
{
	return detail::ReferenceArray<typename CommonType<Args...>::Type, sizeof...(Args)>{ args... };
}


namespace detail
{

template <class ...Ts> struct OverloadedLambda;
template <class T, class ...Ts> struct OverloadedLambda<T, Ts...>
	: public T, OverloadedLambda<Ts...>
{
	OverloadedLambda(T t, Ts ...ts)
		: T(t), OverloadedLambda<Ts...>(ts...) {}
	using T::operator();
	using OverloadedLambda<Ts...>::operator();
};
template <class T> struct OverloadedLambda<T>
	: public T
{
	OverloadedLambda(T t)
		: T(t) {}
	using T::operator();
};

}

template <class ...T> detail::OverloadedLambda<T...> Overload(T ...t) { return detail::OverloadedLambda<T...>{ t... }; }

}

}

#endif