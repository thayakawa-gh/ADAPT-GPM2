﻿//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#ifndef CUF_TEMPLATE_H
#define CUF_TEMPLATE_H

#include <string>
#include <iostream>
#include <limits>

namespace adapt
{

inline namespace cuf
{

//累乗計算クラス。ハッシュ化とか必要に応じて使おう。でもできればconstexpr欲しい。
template <int base, int power>
struct TPower
{
	static_assert(power >= 0, "power must be equal or larger than 0");
	static constexpr int value = TPower<base, power - 1>::value * base;
};
template <int base>
struct TPower<base, 0>
{
	static constexpr int value = 1;
};

template <int N, int ...Ns>
struct IntSum
{
	static constexpr int value = N + IntSum<Ns...>::value;
};
template <int N>
struct IntSum<N>
{
	static constexpr int value = N;
};

//C++20のstd::type_identityの代用。
template <class T>
struct Identity
{
	using Type = T;
};
template <class T>
using IdentityT = typename Identity<T>::Type;

template <class Derived, class Base>
class IsBasedOn
{
private:
	typedef char  Yes;
	typedef struct { char c[2]; } No;

	static constexpr Yes check(const Base&);
	static constexpr No  check(...);

	static const Derived& d;
public:
	static constexpr bool value = sizeof(check(d)) == sizeof(Yes);
};
template <class Base>
class IsBasedOn<void, Base>
{
public:
	static constexpr bool value = false;
};
//Base<T...>をTに依らず継承しているか否かを判定する汎用的なクラスを書くことは出来ないのだろうか。
//あった！あったぞ！できたぞ！すげぇ！テンプレート便利すぎだろこん畜生！
//実は継承関係でなくとも、T<int>に対するT<...>みたいな関係であれば判定できる。
template <class Derived, template <class ...> class Base>
class IsBasedOn_T
{
private:
	typedef char  Yes;
	typedef struct { char c[2]; } No;

	template <class ...U>
	static constexpr Yes check(const Base<U...>&);
	static constexpr No  check(...);

	static const Derived& d;
public:
	static constexpr bool value = sizeof(check(d)) == sizeof(Yes);
};
template <template <class ...> class Base>
class IsBasedOn_T<void, Base>
{
public:
	static constexpr bool value = false;
};
template <class Derived, template <int, class...> class Base>
class IsBasedOn_NT
{
private:
	typedef char  Yes;
	typedef struct { char c[2]; } No;

	template <int N>
	static constexpr Yes check(const Base<N>&);
	//本来はこちらの形式が正しいのだが、
	//Visual Studioのバグか、コンパイル時に"パラメータ展開できねぇよ"と文句を言われる。
	//テンプレート周りがゴミカスなVS2015なので仕方ない。
	//template <int N, class ...U>
	//static const Yes check(const Base<N, U...>&);
	static constexpr No  check(...);

	static const Derived& d;
public:
	static constexpr bool value = sizeof(check(d)) == sizeof(Yes);
};
template <template <int, class ...> class Base>
class IsBasedOn_NT<void, Base>
{
public:
	static constexpr bool value = false;
};
template <class Derived, template <int, std::size_t, class...> class Base>
class IsBasedOn_NNT
{
private:
	typedef char  Yes;
	typedef struct { char c[2]; } No;

	template <int N, int M, class ...U>
	static constexpr Yes check(const Base<N, M, U...>&);
	static constexpr No  check(...);

	static const Derived& d;
public:
	static constexpr bool value = sizeof(check(d)) == sizeof(Yes);
};

//template <...> class T : public Base<0, ...>の形で可変長引数テンプレートのメンバを持つクラスに対し、
//N番目の継承要素へキャストすることで要素を取り出すための関数。
template <int N, template <int, class ...> class Vessel, class ...Members>
inline Vessel<N, Members...>& CastToNth(Vessel<N, Members...>& v)
{
	return v;
}
template <int N, template <int, class ...> class Vessel, class ...Members>
inline const Vessel<N, Members...>& CastToNth(const Vessel<N, Members...>& v)
{
	return v;
}

template <int N>
struct Number
{
	static constexpr int value = N;
	//static constexpr Number<N + n> operator+(constexpr int n);
	//static constexpr Number<N - n> operator-(constexpr int n);
};

template <class ...>
using VoidT = void;

//FlexibleSwitchと似ているが、こちらはループするためのもの。
//0からRoopNum-1までループするfor文のような振る舞いをする。
//Functorの戻り値は無視されるので注意。

template <std::size_t RoopNum, std::size_t N, template <std::size_t CastN> class Functor, bool Continue = (RoopNum > N)>
struct StaticRoop_impl;
template <std::size_t RoopNum, std::size_t N, template <std::size_t CastN> class Functor, bool Continue>
struct StaticRoop_impl
{
	template <class ...Args>
	inline static void apply(Args&& ...args)
	{
		Functor<N>::apply(args...);
		StaticRoop_impl<RoopNum, N + 1, Functor>::apply(std::forward<Args>(args)...);
	}
};
template <std::size_t RoopNum, std::size_t N, template <std::size_t CastN> class Functor>
struct StaticRoop_impl<RoopNum, N, Functor, false>
{
	template <class ...Args>
	inline static void apply(Args&& ...args)
	{}
};
template <std::size_t RoopNum, template <std::size_t CastN> class Functor, class ...Args>
inline void StaticRoop(Args&& ...args)
{
	StaticRoop_impl<RoopNum, 0, Functor>::apply(std::forward<Args>(args)...);
};

//FlexibleSwitchは再帰処理で非効率なので、関数ポインタテーブル版を新たに作りたい。

template <class ...Types>
void WrapSequence(Types&& ...types)
{}

//あるクラスがメンバ関数MemFuncを持つかどうかを判定するHasMemFunc_##MemFunc、
//……を定義するためのマクロ。クラススコープ内で用いるべし。
#define HasMemFunc(MemFunc) \
template <class T> \
struct HasMemFunc_##MemFunc \
{ \
	template <class U> \
	static auto check(U&) ->decltype(std::declval<U&>().MemFunc(), std::true_type()); \
	static auto check(...)->decltype(std::false_type()); \
	typedef decltype(check(std::declval<T&>())) type; \
	static constexpr bool value = type::value; \
};

template <class T>
struct IsFunctor
{
	template <class U>
	static auto check(U&) -> decltype(&U::operator(), std::true_type());
	static auto check(...) -> decltype(std::false_type());
	typedef decltype(check(std::declval<T&>())) type;
	static constexpr bool value = type::value;
};

template <std::size_t N, class Type, class ...Args>
struct Find_impl;
template <std::size_t N, class Type, class ArgHead, class ...Args>
struct Find_impl<N, Type, ArgHead, Args...>
{
	static constexpr std::size_t Index = Find_impl<N + 1, Type, Args...>::Index;
	static constexpr bool value = Find_impl<N + 1, Type, Args...>::value;
};
template <std::size_t N, class Type, class ...Args>
struct Find_impl<N, Type, Type, Args...>
{
	static constexpr std::size_t Index = N;
	static constexpr bool value = true;
};
template <std::size_t N, class Type>
struct Find_impl<N, Type>
{
	static constexpr std::size_t Index = std::numeric_limits<std::size_t>::max();
	static constexpr bool value = false;
};
template <class Type, class ...Args>
struct Find : public Find_impl<0, Type, Args...>
{};

//可変長引数テンプレートから、0から数えてN番目の要素の型を取り出す。GetType<N, elements...>::Typeで用いる。
template <size_t N, class ...Types>
class GetType;
template <size_t N, class TypeHead, class ...TypeBody>
class GetType<N, TypeHead, TypeBody...> : public GetType<N - 1, TypeBody...>
{};
template <size_t N>
class GetType<N>
{
public:
	using Type = void;
};
template <class TypeHead, class ...TypeBody>
class GetType<0, TypeHead, TypeBody...>
{
public:
	using Type = TypeHead;
};
template <class ...TypeBody>
class GetType<-1, TypeBody...>
{
public:
	using Type = void;
};
template <class ...T>
struct TypeList;
template <size_t N, class TypeHead, class ...TypeBody>
class GetType<N, TypeList<TypeHead, TypeBody...>> : public GetType<N - 1, TypeList<TypeBody...>>
{};
template <class TypeHead, class ...TypeBody>
class GetType<0, TypeList<TypeHead, TypeBody...>>
{
public:
	using Type = TypeHead;
};
template <class ...TypeBody>
class GetType<-1, TypeList<TypeBody...>>
{
public:
	using Type = void;
};
template <size_t N>
class GetType<N, TypeList<>>
{
public:
	using Type = void;
};

template <size_t Index, template <size_t, class...> class ...T>
struct GetType_NT;
template <size_t Index, template <size_t, class...> class THead, template <size_t, class...> class ...T>
struct GetType_NT<Index, THead, T...> : public GetType_NT<Index - 1, T...>
{};
template <size_t Index>
struct GetType_NT<Index>
{};
template <template <size_t, class...> class THead, template <size_t, class...> class ...T>
struct GetType_NT<0, THead, T...>
{
	template <size_t N, class ...U>
	using Type = THead<N, U...>;
};

template <size_t Index, template <class...> class ...T>
struct GetType_T;
template <size_t Index, template <class...> class THead, template <class...> class ...T>
struct GetType_T<Index, THead, T...> : public GetType_T<Index - 1, T...>
{};
template <size_t Index>
struct GetType_T<Index>
{};
template <template <class...> class THead, template <class...> class ...T>
struct GetType_T<0, THead, T...>
{
	template <class ...U>
	using Type = THead<U...>;
	template <class U>
	using Type1 = THead<U>;
	template <class U1, class U2>
	using Type2 = THead<U1, U2>;
	template <class U1, class U2, class U3>
	using Type3 = THead<U1, U2, U3>;
};

//int型テンプレート引数を一つだけ持つようなクラスを可変長引数テンプレートに与えたいときに、
//それを一つに纏めるために使う。
//ぶっちゃけInterpreterとTraverser専用。それ以外の使い道なんざねぇよ。
template <template <size_t, class ...> class ...T>
struct UnindexedList
{
	static constexpr std::size_t Size = sizeof...(T);

};
template <class ...List>
struct CatUnindexedList;
template <template <size_t, class...> class ...List1>
struct CatUnindexedList<UnindexedList<List1...>>
{
	using Type = UnindexedList<List1...>;
};
template <template <size_t, class...> class ...List1, template <size_t, class...> class ...List2>
struct CatUnindexedList<UnindexedList<List1...>, UnindexedList<List2...>>
{
	using Type = UnindexedList<List1..., List2...>;
};
template <class List1, class List2, class List3, class ...Lists>
struct CatUnindexedList<List1, List2, List3, Lists...>
{
	using Type = CatUnindexedList<CatUnindexedList<List1, List2>, List3, Lists...>;
};

template <template <size_t, size_t, class ...> class ...T>
struct UnindexedList2
{
public:
	static constexpr std::size_t Size = sizeof...(T);
};
//インデックス持たない版。
template <template <class...> class ...T>
struct UnarguedList
{
public:
	static constexpr std::size_t Size = sizeof...(T);
};
//単なる型の羅列。
template <class ...T>
struct TypeList
{
	static constexpr std::size_t Size = sizeof...(T);
};
template <class ...T>
struct CatTypeList;
template <class ...T1>
struct CatTypeList<TypeList<T1...>>
{
	using Type = TypeList<T1...>;
};
template <class ...T1, class ...T2>
struct CatTypeList<TypeList<T1...>, TypeList<T2...>>
{
	using Type = TypeList<T1..., T2...>;
};
template <class T1, class T2, class T3, class ...Ts>
struct CatTypeList<T1, T2, T3, Ts...>
	: public CatTypeList<typename CatTypeList<T1, T2>::Type, T3, Ts...>
{};
template <class ...T>
using CatTypeListT = typename CatTypeList<T...>::Type;

template <int N, class ...Args>
using GetTypeT = typename GetType<N, Args...>::Type;

template <int Nth, int M, class Type, class Head, class ...Body, std::enable_if_t<Nth != M, std::nullptr_t> = nullptr>
Type GetNthArgument_impl(Head&& head, Body&& ...body)
{
	return GetNthArgument_impl<Nth, M + 1, Body...>(std::forward<Body>(body)...);
}
template <int Nth, int M, class Type, class Head, class ...Body, std::enable_if_t<Nth == M, std::nullptr_t> = nullptr>
Type GetNthArgument_impl(Head&& head, Body&& ...body)
{
	return std::forward<Head>(head);
}
template <int Nth, class ...Args, class Type = GetTypeT<Nth, Args...>>
Type GetNthArgument(Args&& ...args)
{
	return GetNthArgument_impl<Nth, 0, Args...>(std::forward<Args>(args)...);
}

template <class Type_>
class GetInnerType;
template <class Type_>
class GetInnerType
{
public:
	using Type = void;
};
template <template <class> class Type_, class Inner>
class GetInnerType<Type_<Inner>>
{
public:
	using Type = Inner;
};

template <class Type_>
using GetInnerTypeT = typename GetInnerType<Type_>::Type;

namespace detail
{

template <class ...Ts>
struct CommonType_impl;
template <class CommonT>
struct CommonType_impl<CommonT, CommonT>
{
	using Type = CommonT;
	static constexpr bool value = true;
};
template <class CommonT, class T, class ...Ts>
struct CommonType_impl<CommonT, T, Ts...>
{
	static constexpr bool value = false;
};
template <class CommonT, class ...Ts>
struct CommonType_impl<CommonT, CommonT, Ts...> : public CommonType_impl<CommonT, Ts...>
{};

}

template <class ...T>
struct CommonType
{
	using Type = typename detail::CommonType_impl<T...>::Type;
	static constexpr bool value = detail::CommonType_impl<T...>::value;
};
template <class ...T>
using CommonTypeT = typename CommonType<T...>::Type;


//受け取った全てのbool値についてAnd演算を行う。
//全てがtrueであった場合のみValueはtrueとなる。
//こんな機能くらい標準ライブラリに入ってそうだけど、見つけられなかった。
template <bool ...BoolSequence>
struct AndOperationSeq;
template <bool Head, bool ...Body>
struct AndOperationSeq<Head, Body...>
{
	static constexpr bool value = Head && AndOperationSeq<Body...>::value;
};
template <bool Head>
struct AndOperationSeq<Head>
{
	static constexpr bool value = Head;
};
template <>
struct AndOperationSeq<>
{
	static constexpr bool value = false;
};
template <bool B>
struct AndOperationSeqEnabler_impl
{
	using Type = void;
};
template <>
struct AndOperationSeqEnabler_impl<true>
{
	using Type = std::nullptr_t;
};
template <bool ...BoolSequence>
using AndOperationSeqEnablerT = typename AndOperationSeqEnabler_impl<AndOperationSeq<BoolSequence...>::value>::Type;

template <bool B>
using EnableIfT = std::enable_if_t<B, std::nullptr_t>;

//VS2015は変数テンプレート未対応。糞食らえ。
//template <bool ...V>
//constexpr bool AndOperationSeq_v = AndOperationSeq<V...>::Value;

//受け取った全てのbool値についてOr演算を行う。
//いずれか一つでもtrueであった場合、Valueはtrueとなる。
template <bool Head, bool ...Body>
struct OrOperationSeq
{
	static constexpr bool value = Head || OrOperationSeq<Body...>::value;
};
template <bool Head>
struct OrOperationSeq<Head>
{
	static constexpr bool value = Head;
};
template <bool B>
struct OrOperationSeqEnabler_impl
{
	using Type = void;
};
template <>
struct OrOperationSeqEnabler_impl<true>
{
	using Type = std::nullptr_t;
};
template <bool ...BoolSequence>
using OrOperationSeqEnablerT = typename OrOperationSeqEnabler_impl<OrOperationSeq<BoolSequence...>::value>::Type;

namespace detail
{
template <class T>
struct IsReferenceWrapper { static const bool value = false; };
template <class U>
struct IsReferenceWrapper<std::reference_wrapper<U>> { static const bool value = true; };
template <class T, class Type, class Self,
	bool B1 = std::is_base_of<T, std::decay_t<Self>>::value,
	bool B2 = IsReferenceWrapper<std::decay_t<Self>>::value>
	struct InvokeMemFunc_impl
{
	//InvoleMemFunc_impl::applyの戻り値は本来decltype(auto)で十分なはずだが、
	//Visual Studioのバグか、限定的な状況下で戻り値の型を正しく推定してくれない時があったので、
	//この書き方にしておく。
	template <class ...Args>
	static auto apply(Type T::* f, Self&& self, Args&& ...args)
		->decltype(((*std::forward<Self>(self)).*f)(std::forward<Args>(args)...))
	{
		return ((*std::forward<Self>(self)).*f)(std::forward<Args>(args)...);
	}
};
template <class T, class Type, class Self, bool B2>
struct InvokeMemFunc_impl<T, Type, Self, true, B2>
{
	template <class ...Args>
	static auto apply(Type T::* f, Self&& self, Args&& ...args)
		->decltype((std::forward<Self>(self).*f)(std::forward<Args>(args)...))
	{
		return (std::forward<Self>(self).*f)(std::forward<Args>(args)...);
	}
};
template <class T, class Type, class Self>
struct InvokeMemFunc_impl<T, Type, Self, false, true>
{
	template <class ...Args>
	static auto apply(Type T::* f, Self&& self, Args&& ...args)
		->decltype((self.get().*f)(std::forward<Args>(args)...))
	{
		return (self.get().*f)(std::forward<Args>(args)...);
	}
};

template <class T, class Type, class Self, class ...Args>
auto Invoke_impl(Type T::* f, Self&& self, Args&& ...args) ->decltype(auto)
{
	return InvokeMemFunc_impl<T, Type, Self>::
		apply(std::forward<decltype(f)>(f), std::forward<Self>(self), std::forward<Args>(args)...);
}
template <class Func, class ...Args>
auto Invoke_impl(Func&& f, Args&& ...t) ->decltype(auto)
{
	return std::forward<Func>(f)(std::forward<Args>(t)...);
}

template <class AlwaysVoid, class, class...>
struct InvokeResult { };
template <typename F, typename...Args>
struct InvokeResult<decltype(void(detail::Invoke_impl(std::declval<F>(), std::declval<Args>()...))), F, Args...>
{
	using Type = decltype(detail::Invoke_impl(std::declval<F>(), std::declval<Args>()...));
};

}
//std::invokeの代替。
//あちらと違いメンバ変数には非対応。
//メンバ変数を関数風に呼び出すとか変態的な使い方が必要になるとは思えん。
template <class F, class... ArgTypes>
struct InvokeResult : detail::InvokeResult<void, F, ArgTypes...> {};
template <class Func, class ...Args>
decltype(auto) Invoke(Func&& f, Args&& ...args)
{
	return detail::Invoke_impl(std::forward<Func>(f), std::forward<Args>(args)...);
}

//std::applyがmsvc2015ではまだ使えないので、
//代替となる関数を用意しておく。
namespace detail
{
template <class Func, class Tuple, std::size_t ...Indices>
decltype(auto) Apply_impl(Func&& f, Tuple&& t, std::index_sequence<Indices...>)
{
	return Invoke(std::forward<Func>(f), std::get<Indices>(t)...);
}
}
template <class Func, class Tuple>
decltype(auto) Apply(Func&& f, Tuple&& t)
{
	return detail::Apply_impl(std::forward<Func>(f), std::forward<Tuple>(t),
							  std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>());
}

//c++14ではremove_cvrefがないので、その代用品。
template <class T>
struct RemoveCVRef
{
	using Type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <class T>
using RemoveCVRefT = typename RemoveCVRef<T>::Type;

namespace detail
{

template <class Function, bool B = IsFunctor<Function>::value>
struct FunctionTraits_impl;
template <class Function>
struct FunctionTraits_impl<Function, true>
{
	using ArgTypes = typename FunctionTraits_impl<decltype(&Function::operator())>::ArgTypes;
	using RetType = typename FunctionTraits_impl<decltype(&Function::operator())>::RetType;
};
template <class R, class T, class ...Args>
struct FunctionTraits_impl<R(T::*)(Args...) const, false>
{
	using ArgTypes = TypeList<Args...>;
	using RetType = R;
};
template <class R, class T, class ...Args>
struct FunctionTraits_impl<R(T::*)(Args...), false>
{
	using ArgTypes = TypeList<Args...>;
	using RetType = R;
};
template <class R, class ...Args>
struct FunctionTraits_impl<R(*)(Args...), false>
{
	using ArgTypes = TypeList<Args...>;
	using RetType = R;
};
template <class R, class ...Args>
struct FunctionTraits_impl<R(Args...), false>
{
	using ArgTypes = TypeList<Args...>;
	using RetType = R;
};

}

//関数ポインタ、メンバ関数ポインタ、関数オブジェクト、いずれかを受け取り、
//その引数と戻り値の方を取得する。
template <class Function>
struct FunctionTraits
{
	using ArgTypes = typename detail::FunctionTraits_impl<Function>::ArgTypes;
	using RetType = typename detail::FunctionTraits_impl<Function>::RetType;
};

}

}
#endif