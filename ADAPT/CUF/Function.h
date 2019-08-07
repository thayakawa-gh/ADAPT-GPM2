//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#ifndef CUF_FUNCTION_H
#define CUF_FUNCTION_H

#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <sstream>
#include <functional>

namespace adapt
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
	return std::move(res);
}

inline std::vector<std::string> SplitStr(const std::string& str)
{
	std::vector<std::string> res;
	std::istringstream ss(str);
	std::string buffer;
	while (ss >> buffer) res.push_back(buffer);
	return std::move(res);
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
	return std::move(res);
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
/*template <int Size, class ...Args>
std::string Format(const char(&format)[Size], Args&& ...args)
{
	char buf[2 * Size + 256];
	sprintf(buf, format, std::forward<Args>(args)...);
	return std::string(buf);
}*/

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
	std::is_convertible<const char*, std::string>::value;
}
template <class T, std::enable_if_t<std::is_convertible<T, std::string>::value, std::nullptr_t> = nullptr>
std::string ToString(const T& c)
{
	return std::string(c);
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
constexpr auto InfiniteProduct(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccPlus(), std::forward<T>(v)...);
}
template <class ...T>
constexpr auto AndAll(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccPlus(), std::forward<T>(v)...);
}
template <class ...T>
constexpr auto OrAll(T&& ...v)
{
	return detail::Accumulate_impl(detail::AccPlus(), std::forward<T>(v)...);
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
return _pclose(stream); \
}
#endif

}

#endif