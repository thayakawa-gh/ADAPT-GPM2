//
// std::string Format(const std::string& format, const Args& ...args);
// が定義されている。printf風に引数を与えれば良い。
// フォーマット指定が正しい場合、printf系関数とできるだけ同じ出力になるよう設計されているが、
// 指定に誤りがある場合はこの限りではない。
//
// 指定と異なる型が与えられている場合、それがキャスト可能な型であれば勝手にキャストされる。
// したがって、例えば%dにlong long intを与えたりするとintにキャストされ4バイト分切り詰められてしまう。
//
// もし厳密な型チェックを行いたいのなら、
// このファイルをincludeする前に
// #define REQUIRE_STRICT_TYPE_MATCHING
// とする。この場合、指定子、修飾子と引数型の厳密な一致が要求されるようになる。
// 例えば、%dにlong long intを与えると!TYPE_MISMATCH!が出力される（%dはint型用なので）。
// %uにintを与えても!TYPE_MISMATCH!である。unsigned intを与えなければならない。
//
// 変換指定はprintf等と同じく
// %[フラグ][最小フィールド幅][.精度][長さ修飾子]変換指定子
// である。
//
// * フラグ
// - : 左寄せ
// + : 常に符号を出力
// # : 基数を表すプレフィックスの出力等
// 0 : 0埋め
//
// * 長さ修飾子
// なし: int、unsigned int、float、double
// hh: char
// h : short
// l : longまたはfloat、double（%fと%lfはprintfにおいては全く同じなので、それに習った）
// ll: long long
// L : long double
// z : size_t
// ? : 整数または浮動少数点全てに対応
//
// * 変換指定子
// d, i : 10進符号付き整数
// u    : 10進符号無し整数
// o    : 8進符号無し整数
// x, X : 16進符号無し整数（Xは大文字で出力）
// e, E : 指数形式浮動小数点数（Eは大文字で出力）
// f, F : 小数形式浮動小数点数（Fは大文字で出力）
// a, A : 16進浮動小数点（Aは大文字で出力）
// b    : bool値をtrueまたはfalseの文字で出力
// c    : char一文字
// s    : 文字列
// %    : 文字の"%"
//
// ある程度printfの機能を再現したが以下のものは用意するのが面倒くさかったので対応していない。誰か作って。
// * %...pによるポインタ値
// * %...nによる出力済み文字数の格納
// * 長さ修飾子j、t
// * %...lcによるワイド文字
//
// その他、他のライブラリのFormat関数で見られた便利機能を多少追加した。
// * 整数、浮動小数点に対して使える万能の長さ修飾子"?"。
//   長さ修飾子指定が面倒くさい場合、%?dとか%?fとすると勝手に適切な型として出力する。boolにも対応している。
// * %sの文字列をstd::stringで与える機能。C++ならconst char*よりはこちらが標準なので。
// * %bでbool値をtrueまたはfalseの文字列で出力する。
//
// 既知の問題
// * %.0eのように指数表示に対して精度0を指定したとき、小数点以下が表示されてしまう。
//   std::setprecision(0)が何故かstd::scientificに対して働いていないらしい。
//   これが標準ライブラリの仕様なのか、それともバグなのかは不明。
//

#ifndef CUF_FORMAT_H
#define CUF_FORMAT_H

#include <sstream>
#include <cstddef>
#include <cctype>
#include <type_traits>
#include <exception>

namespace adapt
{

inline namespace cuf
{

namespace detail
{

inline char Parse(std::ostream& str, std::string::const_iterator& it, const std::string::const_iterator& itend)
{
	while (it != itend)
	{
		if (*it == '0') str.fill('0'), str.setf(std::ios::internal);
		else if (*it == '+') str.setf(std::ios::showpos);
		else if (*it == '-') str.setf(std::ios::left);
		else if (*it == '#') str.setf(std::ios::showpoint | std::ios::showbase);
		else break;
		++it;
	}

	int digit = 0;
	while (it != itend && std::isdigit(*it))
	{
		digit = 10 * digit + *it - '0';
		++it;
	}
	if (digit != 0) str.width(digit);

	if (it != itend && *it == '.')
	{
		++it;
		int dec = 0;
		while (it != itend && std::isdigit(*it))
		{
			dec = 10 * dec + *it - '0';
			++it;
		}
		if (dec >= 0) str.precision(dec);
	}

	char mod = 0;
	if (it != itend)
	{
		switch (*it)
		{
		case 'z':
		case 'L':
		case '?': mod = *it++; break;
		case 'l':
			if (*(++it) == 'l') mod = 'm', ++it;
			else mod = 'l';
			break;
		case 'h':
			if (*(++it) == 'h') mod = 'g', ++it;
			else mod = 'h';
			break;
		}
	}
	return mod;
}

#ifdef REQUIRE_STRICT_TYPE_MATCHING
//厳密な型一致を要求する。
//指定子と型が一致しない場合例外を投げる。
template <class Type, class Value, bool IsMatched = std::is_same<Type, Value>::value>
struct Write_impl
{
	static void f(std::ostream& str, const Value& v) { str << v; }
};
template <>
struct Write_impl<double, float, false>
{
	//doubleの場合、printfの仕様に準拠するため、
	//floatからのキャストは常に許されなければならない。
	static void f(std::ostream& str, double v) { str << v; }
};
#else
//型の不一致を許し、キャスト可能な場合はstatic_castする。
template <class Type, class Value, bool IsConvertible = std::is_convertible<Value, Type>::value>
struct Write_impl { static void f(std::ostream& str, const Value& v) { str << static_cast<Type>(v); } };
//boolのときだけは、阿呆なmsvc2015がキャスト時に警告を発しやがるので、特殊化して回避する。
//msvc2017以降は発さないよう修正されたらしい。
template <class Value>
struct Write_impl<bool, Value, true> { static void f(std::ostream& str, const Value& v) { str << (v != 0); } };
#endif
template <class Type, class Value>
struct Write_impl<Type, Value, false> { static void f(std::ostream&, const Value&) { throw std::exception(); } };

template <class Type, class Value>
void Write(std::ostream& str, const Value& v)
{
	Write_impl<Type, Value>::f(str, v);
}

template <class Value, bool IsArithmetic = std::is_arithmetic<Value>::value, bool IsIntegral = std::is_integral<Value>::value, bool IsSigned = std::is_signed<Value>::value>
struct WriteAnyInt_impl
{
	//正しくない動作。
	static void f(std::ostream&, Value) { throw std::exception(); }
};
template <class Value>
struct WriteAnyInt_impl<Value, true, true, true>
{
	//正しい動作。
	static void f(std::ostream& str, Value v) { str << v; }
};
template <>
struct WriteAnyInt_impl<signed char, true, true, true>
{
	//整数かつsignedであるような正しい動作だが、
	//charなので文字出力にならないよう特殊化する。
	static void f(std::ostream& str, signed char v) { str << +v; }
};
template <>
struct WriteAnyInt_impl<char, true, true, true>
{
	//整数かつsignedであるような正しい動作だが、
	//charなので文字出力にならないよう特殊化する。
	//charとsigned charは別扱いなので特殊化も分離する。
	//charは環境によってはunsignedである場合もあるが、
	//その場合はIsSignedがfalseになるので型の不一致扱いとなり、この関数は呼ばれない。
	static void f(std::ostream& str, char v) { str << +v; }
};
template <>
struct WriteAnyInt_impl<bool, true, true, false>
{
	//boolはsignedでもunsignedでもないが、整数として扱うため特殊化する。
	static void f(std::ostream& str, bool v) { str << v; }
};
#ifndef REQUIRE_STRICT_TYPE_MATCHING
template <class Value>
struct WriteAnyInt_impl<Value, true, true, false>
{
	//整数だがsignedではない
	static void f(std::ostream& str, Value v) { str << +static_cast<std::make_signed_t<Value>>(v); }
};
template <class Value, bool IsSigned>
struct WriteAnyInt_impl<Value, true, false, IsSigned>
{
	//signedだが非整数
	static void f(std::ostream& str, Value v) { str << static_cast<long long>(v); }
};
#endif
template <class Value>
void WriteAnyInt(std::ostream& str, Value v)
{
	WriteAnyInt_impl<Value>::f(str, v);
}

template <class Value, bool IsArithmetic = std::is_arithmetic<Value>::value, bool IsIntegral = std::is_integral<Value>::value, bool IsUnsigned = std::is_unsigned<Value>::value>
struct WriteAnyUnsignedInt_impl
{
	//正しくない動作。
	static void f(std::ostream&, Value) { throw std::exception(); }
};
template <class Value>
struct WriteAnyUnsignedInt_impl<Value, true, true, true>
{
	//正しい動作。
	static void f(std::ostream& str, Value v) { str << v; }
};
template <>
struct WriteAnyUnsignedInt_impl<unsigned char, true, true, true>
{
	//整数かつsignedであるような正しい動作だが、
	//unsigned charなので文字出力にならないよう特殊化する。
	static void f(std::ostream& str, unsigned char v) { str << (unsigned int)v; }
};
template <>
struct WriteAnyUnsignedInt_impl<char, true, true, true>
{
	//整数かつunsignedであるような正しい動作だが、
	//charなので文字出力にならないよう特殊化する。
	//charがsignedかunsignedかは環境依存なので、場合によってはこれが必要になることがある。
	static void f(std::ostream& str, char v) { str << +v; }
};
template <>
struct WriteAnyUnsignedInt_impl<bool, true, true, false>
{
	//boolはsignedでもunsignedでもないが、整数として扱うため特殊化する。
	static void f(std::ostream& str, bool v) { str << v; }
};
#ifndef REQUIRE_STRICT_TYPE_MATCHING
template <class Value>
struct WriteAnyUnsignedInt_impl<Value, true, true, false>
{
	//整数だがunsignedではない
	static void f(std::ostream& str, Value v) { str << +static_cast<std::make_unsigned_t<Value>>(v); }
};
template <class Value, bool IsUnsigned>
struct WriteAnyUnsignedInt_impl<Value, true, false, IsUnsigned>
{
	//非整数、必然的にunsignedではない。
	static void f(std::ostream& str, Value v) { str << static_cast<unsigned long long>(v); }
};
#endif
template <class Value>
void WriteAnyUnsignedInt(std::ostream& str, Value v)
{
	WriteAnyUnsignedInt_impl<Value>::f(str, v);
}

template <class Value, bool IsArithmetic = std::is_arithmetic<Value>::value, bool IsFloat = std::is_floating_point<Value>::value>
struct WriteAnyFloat_impl
{
	static void f(std::ostream&, Value) { throw std::exception(); }
};
template <class Value>
struct WriteAnyFloat_impl<Value, true, true>
{
	static void f(std::ostream& str, Value v) { str << v; }
};
#ifndef REQUIRE_STRICT_TYPE_MATCHING
template <class Value>
struct WriteAnyFloat_impl<Value, true, false>
{
	//ゆるい型一致のときは整数型からもdoubleにキャストされる。
	static void f(std::ostream& str, Value v) { str << static_cast<double>(v); }
};
#endif
template <class Value>
void WriteAnyFloat(std::ostream& str, Value v)
{
	WriteAnyFloat_impl<Value>::f(str, v);
}

inline void WriteAnyString(std::ostream& str, const std::string& v)
{
	str << v;
}
inline void WriteAnyString(std::ostream& str, const char* v)
{
	str << v;
}
inline void WriteAnyString(std::ostream&, ...)
{
	throw std::exception();
}
template <class Value>
void WriteInt(std::ostringstream& str, const Value& arg, char mod)
{
	switch (mod)
	{
	case 0: Write<int>(str, arg); break;
	case 'l': Write<long>(str, arg); break;
	case 'm': Write<long long>(str, arg); break;
	case 'h': Write<short>(str, arg); break;
	case 'g': Write<char>(str, arg); break;
	case '?': WriteAnyInt(str, arg); break;
	default: throw std::exception(); break;
	}
}
template <class Value>
void WriteUnsignedInt(std::ostringstream& str, const Value& arg, char mod)
{
	switch (mod)
	{
	case 'z': Write<std::size_t>(str, arg); break;
	case 'm': Write<unsigned long long>(str, arg); break;
	case 'l': Write<unsigned long>(str, arg); break;
	case 0: Write<unsigned int>(str, arg); break;
	case 'h': Write<unsigned short>(str, arg); break;
	case 'g': Write<unsigned char>(str, arg); break;
	case '?': WriteAnyUnsignedInt(str, arg); break;
	default: throw std::exception(); break;
	}
}
template <class Value>
void WriteFloat(std::ostringstream& str, const Value& arg, char mod)
{
	switch (mod)
	{
	case 0:
	case 'l': Write<double>(str, arg); break;
	case 'L': Write<long double>(str, arg); break;
	case '?': WriteAnyFloat(str, arg); break;
	default: throw std::exception(); break;
	}
}

template <class Value>
void AddArg(std::string& result, std::string::const_iterator& it, const std::string::const_iterator& itend, const Value& arg)
{
	std::ostringstream str;
	try
	{
		char mod = 0;
		mod = Parse(str, it, itend);
		if (it != itend)
		{
			char type = *it++;
			switch (type)
			{
			case 'd':
			case 'i':
				str << std::dec;
				WriteInt(str, arg, mod);
				break;
			case 'u':
				str << std::dec;
				WriteUnsignedInt(str, arg, mod);
				break;
			case 'o':
				str << std::oct;
				WriteUnsignedInt(str, arg, mod);
				break;
			case 'x':
				str << std::hex;
				WriteUnsignedInt(str, arg, mod);
				break;
			case 'X':
				str << std::hex << std::uppercase;
				WriteUnsignedInt(str, arg, mod);
				break;
			case 'f':
				str << std::fixed;
				WriteFloat(str, arg, mod);
				break;
			case 'e':
				str << std::scientific;
				WriteFloat(str, arg, mod);
				break;
			case 'E':
				str << std::scientific << std::uppercase;
				WriteFloat(str, arg, mod);
				break;
			case 'g':
				str << std::defaultfloat;
				WriteFloat(str, arg, mod);
				break;
			case 'G':
				str << std::defaultfloat << std::uppercase;
				WriteFloat(str, arg, mod);
				break;
			case 'a':
				str << std::hexfloat;
				WriteFloat(str, arg, mod);
				break;
			case 'A':
				str << std::hexfloat << std::uppercase;
				WriteFloat(str, arg, mod);
				break;
			case 's':
				WriteAnyString(str, arg);
				break;
			case 'c':
				Write<char>(str, arg);
				break;
			case 'b':
				str << std::boolalpha;
				Write<bool>(str, arg);
				break;
			default:
				str << type;
			}
		}
	}
	catch (const std::exception&)
	{
		str << "!TYPE_MISMATCH!";
	}
	result += str.str();
}
inline void AddArg(std::string& result, std::string::const_iterator& it, const std::string::const_iterator& itend)
{
	std::ostringstream str;
	//char mod = 0;
	/*mod = */Parse(str, it, itend);
	if (it != itend)
	{
		str << "!NO_ARGUMENTS!";
		++it;
	}
	result += str.str();
}

inline void Format_impl(std::string& result,
						std::string::const_iterator& it, std::string::const_iterator& itend)
{
	while (it != itend)
	{
		switch (*it)
		{
		case '%':
			++it;
			if (*it != '%')
			{
				if (it != itend)
					AddArg(result, it, itend);
				break;
			}
		default:
			result += *it;
			++it;
		}
	}
}
template <class Value, class ...Values>
void Format_impl(std::string& result,
				 std::string::const_iterator& it, std::string::const_iterator& itend,
				 const Value& value, const Values& ...values)
{
	while (it != itend)
	{
		switch (*it)
		{
		case '%':
			++it;
			if (*it != '%')
			{
				if (it != itend)
					AddArg(result, it, itend, value);
				return Format_impl(result, it, itend, values...);
			}
		default:
			result += *it;
			++it;
		}
	}
}

}

template <class ...Values>
std::string Format(const std::string& format, const Values& ...values)
{
	std::string::const_iterator it = format.begin();
	std::string::const_iterator itend = format.end();
	std::string result;
	detail::Format_impl(result, it, itend, values...);
	return result;
}

}

}

#endif