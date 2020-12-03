#ifndef CUF_SWITCH_H
#define CUF_SWITCH_H

#include <tuple>
#include <string>
#include <iostream>
#include <ADAPT/CUF/Macro.h>
#include <ADAPT/CUF/Exception.h>

namespace adapt
{

inline namespace cuf
{

//impl2とimpl3が分離しているのは、Functor用IndicesとArg用Indicesの展開が重複した際にVisual Studio 2017がエラーを出すため。バグっぽい。
namespace detail
{
template<std::size_t N, class Result, template <std::size_t> class Functor, class Tuple, std::size_t ...ArgIndices>
Result TabulationSwitch_impl3(Tuple&& t, std::index_sequence<ArgIndices...>)
{
	return Functor<N>::apply(std::get<ArgIndices>(std::forward<Tuple>(t))...);
}
template<std::size_t N, class Result, template <std::size_t> class Functor, class Tuple, class AIS>
Result TabulationSwitch_impl2(Tuple&& t, AIS ais)
{
	return TabulationSwitch_impl3<N, Result, Functor>(std::forward<Tuple>(t), ais);
}

template<std::size_t SwitchNum, class Result, template <std::size_t> class Functor, class Tuple, std::size_t ...Indices, class AIS>
Result TabulationSwitch_impl(std::size_t n, Tuple&& t, std::index_sequence<Indices...>, AIS ais)
{
	Result(*func[SwitchNum])(Tuple&&, AIS) =
	{
		TabulationSwitch_impl2<Indices, Result, Functor, Tuple, AIS>...
	};
	return func[n](std::forward<Tuple>(t), ais);
}
}
template<std::size_t SwitchNum, template <std::size_t> class Functor, class ...Args>
decltype(auto) TabulationSwitch(std::size_t n, Args&& ...args)
{
	return detail::TabulationSwitch_impl<SwitchNum, decltype(std::declval<Functor<0>>().apply(std::forward<Args>(args)...)), Functor>(
		n, std::forward_as_tuple(std::forward<Args>(args)...), std::make_index_sequence<SwitchNum>(), std::make_index_sequence<sizeof...(Args)>());
}

namespace detail
{

template <std::size_t SwitchNum, std::size_t N, class Result, template <std::size_t> class Functor, bool B = (N < SwitchNum)>
	struct FixedLengthSwitch_wrapper
	{
		template <class ...Args>
		static Result apply(Args&& ...args)
		{
			return Functor<N>::apply(std::forward<Args>(args)...);
		}
	};
	template <std::size_t SwitchNum, std::size_t N, class Result, template <std::size_t> class Functor>
	struct FixedLengthSwitch_wrapper<SwitchNum, N, Result, Functor, false>
	{
		template <class ...Args>
		static Result apply(Args&& ...)
		{
			throw OutOfRange("");
		}
	};

}
#define PSEUDOVARIABLESWITCH_FUNC(n)\
case n: return detail::FixedLengthSwitch_wrapper\
<SwitchNum, n, decltype(std::declval<Functor<0>>().apply(std::forward<Args>(args)...)), Functor>::\
apply(std::forward<Args>(args)...);

//TabulationSwitchも関数ポインタを使う以上は非効率。
//どうしても効率化しなければならないときのために
//固定長Switch版も用意しておく。
//そのうち特殊化して任意のSwitchNumに対応するように書き直したい。
template<std::size_t SwitchNum, template <std::size_t> class Functor, class ...Args>
decltype(auto) FixedLengthSwitch(size_t n, Args&& ...args)
{
	switch (n)
	{
		CUF_REPEAT_FOR(100, PSEUDOVARIABLESWITCH_FUNC)
	}
	throw OutOfRange("");
}
#undef PSEUDOVARIABLESWITCH_FUNC

}

}

#endif
