//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#ifndef CUF_KEYWORD_ARGS_H
#define CUF_KEYWORD_ARGS_H

#include <ADAPT/CUF/Template.h>
#include <ADAPT/CUF/Macro.h>
#include <ADAPT/CUF/Exception.h>
#include <tuple>

namespace adapt
{

//関数の引数にPythonっぽいキーワード可変長引数を作るための補助クラス。
//func(param1 = x, param2 = y);みたいな呼び出しをしたい.

namespace detail
{

template <class Name, class Type, class Tag>
struct KeywordValue
{
	using _Type = Type;
	using _Tag = Tag;

	KeywordValue(_Type v) : mValue(std::forward<_Type>(v)) {}
	_Type Get() { return std::forward<_Type>(mValue); }
private:
	_Type mValue;
};
template <class Name, class Type, class Tag>
struct KeywordName;
template <class Name, class Type, class Tag>
struct KeywordName
{
	using _Type = Type;
	using _Value = KeywordValue<Name, Type, Tag>;
	using _Tag = Tag;

	constexpr KeywordName() {}
	constexpr _Value operator=(_Type v) const { return _Value(std::forward<_Type>(v)); }
};
template <class Name, class Tag>
struct KeywordName<Name, void, Tag>
{
	using _Type = void;
	using _Value = KeywordName<Name, void, Tag>;
	using _Tag = Tag;
};

template <class Keyword, class Arg1,
	class Dummy = void,
	bool B = !std::is_convertible<Arg1, typename Keyword::_Type>::value,
	std::enable_if_t<B, std::nullptr_t> = nullptr>
std::remove_reference_t<typename Keyword::_Type> GetDefault(Keyword keyword, Arg1&& arg1)
{
	throw InvalidArg("Default value does not exist.");
}
template <class Keyword, class Arg1,
	bool B = std::is_convertible<Arg1, typename Keyword::_Type>::value,
	std::enable_if_t<B, std::nullptr_t> = nullptr>
std::remove_reference_t<typename Keyword::_Type> GetDefault(Keyword keyword, Arg1&& arg1)
{
	return std::forward<Arg1>(arg1);
}
template <class Keyword, class Arg1, class ...Args,
		  bool B = (sizeof...(Args) != 0),
		  std::enable_if_t<B, std::nullptr_t> = nullptr>
std::remove_reference_t<typename Keyword::_Type> GetDefault(Keyword keyword, Arg1&&, Args&& ...args)
{
	return GetDefault(std::forward<Keyword>(keyword), std::forward<Args>(args)...);
}

template <class keyword>
struct GetKeywordArgInfo
{
	//using Name = void;
	using Value = void;
	using Arg = void;
};
template <class Name_, class Tag_>
struct GetKeywordArgInfo<KeywordName<Name_, void, Tag_>>
{
	//using Name = KeywordName<Name_, void>;
	using Value = void;
	using Arg = KeywordName<Name_, void, Tag_>;
};
template <class Name_, class Type_, class Tag_>
struct GetKeywordArgInfo<KeywordName<Name_, Type_, Tag_>>
{
	//using Name = KeywordName<Name_, Type_>;
	using Value = typename KeywordName<Name_, Type_, Tag_>::_Value;
	using Arg = Value;
};

template <std::size_t KeyIndex>
struct GetKeywordArg_impl
{
	//キーワード引数が与えられている場合。
	template <class Keyword, class ...Args>
	static typename Keyword::_Type f(Keyword, Args&& ...args)
	{
		return std::get<KeyIndex>(std::forward_as_tuple(std::forward<Args>(args)...)).Get();
	}
};
template <>
struct GetKeywordArg_impl<std::numeric_limits<std::size_t>::max()>
{
	//キーワード引数が与えられていない場合。indexはstd::size_tの最大値になる。
	//デフォルト値が引数の最後に与えられている場合はそれを返し、
	//なければstatic_assertでコンパイルエラーにする。
	template <class Keyword, class ...Args>
	static std::remove_reference_t<typename Keyword::_Type> f(Keyword k, Args&& ...args)
	{
		return detail::GetDefault(k, std::forward<Args>(args)...);
	}
};

template <class Option, class Tag>
struct IsTaggedWith
{
	static constexpr bool value = std::is_base_of<typename Option::_Tag, Tag>::value;
};

}

template <class Type>
struct IsKeyword_impl
{
	static const bool value = IsBasedOn_T<RemoveCVRefT<Type>, detail::KeywordName>::value ||
		IsBasedOn_T<RemoveCVRefT<Type>, detail::KeywordValue>::value;
};
template <class Type>
struct IsKeyword
{
	static const bool value = IsKeyword_impl<Type>::value;
};
template <class ...Types>
struct AreAllKeywords
{
	static const bool value = AndOperationSeq<IsKeyword<Types>::value...>::value;
};
template <>
struct AreAllKeywords<>
{
	static const bool value = true;
};

#define CUF_DEFINE_KEYWORD_OPTION(NAME)\
constexpr auto NAME = adapt::detail::KeywordName<struct _##NAME, void, void>();

#define CUF_DEFINE_KEYWORD_OPTION_WITH_TAG(NAME, TAG)\
constexpr auto NAME = adapt::detail::KeywordName<struct _##NAME, void, TAG>();

#define CUF_DEFINE_KEYWORD_OPTIONAL_ARG(NAME, TYPE)\
constexpr auto NAME = adapt::detail::KeywordName<struct _##NAME, TYPE, void>();

#define CUF_DEFINE_KEYWORD_OPTIONAL_ARG_WITH_TAG(NAME, TYPE, TAG)\
constexpr auto NAME = adapt::detail::KeywordName<struct _##NAME, TYPE, TAG>();

#define CUF_TAGGED_ARGS_ENABLER(OPTIONS, TAG)\
bool CUF_TAG_IS_BASE_OF_ARGTAG = (sizeof...(OPTIONS) == 0||AndOperationSeq<adapt::detail::IsTaggedWith<OPTIONS, TAG>::value...>::value),\
std::enable_if_t<CUF_TAG_IS_BASE_OF_ARGTAG, std::nullptr_t> = nullptr

template <class Keyword, class ...Args>
constexpr bool KeywordExists(Keyword, Args&& ...args)
{
	return Find<typename detail::GetKeywordArgInfo<Keyword>::Arg, RemoveCVRefT<Args>...>::value;
}
template <class Keyword, class ...Args>
constexpr bool KeywordExists(Keyword, std::tuple<Args...> args)
{
	return Find<typename detail::GetKeywordArgInfo<Keyword>::Arg, RemoveCVRefT<Args>...>::value;
}

template <class Keyword, class ...Args>
typename decltype(auto) GetKeywordArg(Keyword k, Args&& ...args)
{
	//キーワード引数が与えられている場合に呼ばれる。
	//該当するキーワードから値を取り出して返す。
	//同じキーワードが複数与えられている場合、先のもの（左にあるもの）が優先される。
	return detail::GetKeywordArg_impl<
		Find<typename detail::GetKeywordArgInfo<Keyword>::Value, RemoveCVRefT<Args>...>::Index>::
		f(k, std::forward<Args>(args)...);
}
template <class Keyword, class ...Args>
typename decltype(auto) GetKeywordArg(Keyword k, std::tuple<Args...> args)
{
	//キーワード引数が与えられている場合に呼ばれる。
	//該当するキーワードから値を取り出して返す。
	//同じキーワードが複数与えられている場合、先のもの（左にあるもの）が優先される。
	return Apply(detail::GetKeywordArg_impl<
		Find<typename detail::GetKeywordArgInfo<Keyword>::Value, RemoveCVRefT<Args>...>::Index>::
				 f, std::tuple_cat(std::make_tuple(k), std::forward<std::tuple<Args...>>(args)));
}
template <class Keyword, class ...Args, class DEFAULT>
typename decltype(auto) GetKeywordArg(Keyword k, std::tuple<Args...> args, DEFAULT&& def)
{
	//キーワード引数が与えられている場合に呼ばれる。
	//該当するキーワードから値を取り出して返す。
	//同じキーワードが複数与えられている場合、先のもの（左にあるもの）が優先される。
	return Apply(detail::GetKeywordArg_impl<
				 Find<typename detail::GetKeywordArgInfo<Keyword>::Value, RemoveCVRefT<Args>...>::Index>::
				 f, std::tuple_cat(std::make_tuple(k), std::forward<std::tuple<Args...>>(args),
								   std::forward_as_tuple<DEFAULT>(def)));
}

}

#endif