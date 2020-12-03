#ifndef CUF_VARIANT_H
#define CUF_VARIANT_H

#include <type_traits>
#include <ADAPT/CUF/Template.h>
#include <ADAPT/CUF/Exception.h>
#include <ADAPT/CUF/Switch.h>

//動作未確認。

namespace adapt
{

inline namespace cuf
{

namespace detail
{

template <std::size_t Index, class Storage_, class ...Types>
struct VariantStorer_impl;
template <std::size_t Index, class Storage_, class Head, class ...Body>
struct VariantStorer_impl<Index, Storage_, Head, Body...> : public VariantStorer_impl<Index + 1, Storage_, Body...>
{
	using Storage = Storage_;
	using Base = VariantStorer_impl<Index + 1, Storage, Body...>;

	static void Assign(Storage& s, Head&& h)
	{
		new (&s.mStorage) Head(std::move(h));
		s.mIndex = Index;
	}
	static void Assign(Storage& s, const Head& h)
	{
		new (&s.mStorage) Head(h);
		s.mIndex = Index;
	}
	using Base::Assign;

	template <class ...Args>
	static void Emplace(IndexConstant<Index>, Storage& s, Args&& ...args)
	{
		new (&s.mStorage) Head(std::forward<Args>(args)...);
		s.mIndex = Index;
	}
	using Base::Emplace;

	static Head& Get(IndexConstant<Index>, Storage& s)
	{
		return (*reinterpret_cast<Head*>(&s.mStorage));
	}
	static const Head& Get(IndexConstant<Index>, const Storage& s)
	{
		return (*reinterpret_cast<const Head*>(&s.mStorage));
	}
	using Base::Get;

	static void Destroy(IndexConstant<Index>, Storage& s)
	{
		(reinterpret_cast<Head&>(s.mStorage)).~Head();
		s.mIndex = Storage::Size;
	}
	using Base::Destroy;

};
template <std::size_t Index, class Storage>
struct VariantStorer_impl<Index, Storage>
{
	static void Assign() {}
	static void Emplace() {}
	static void Get() {}
	static void Destroy() {}
};

template <class ...Body>
struct VariantStorageSize;
template <class Head, class ...Body>
struct VariantStorageSize<Head, Body...>
{
	static constexpr size_t Length =
		sizeof(Head) > VariantStorageSize<Body...>::Length ? sizeof(Head) : VariantStorageSize<Body...>::Length;
	static constexpr size_t Align =
		alignof(Head) > VariantStorageSize<Body...>::Align ? alignof(Head) : VariantStorageSize<Body...>::Align;
};
template <>
struct VariantStorageSize<>
{
	static constexpr std::size_t Length = 0;
	static constexpr std::size_t Align = 0;
};
template <class ...Types>
struct VariantStorage
{
	template <std::size_t Index, class Storage_, class ...Types_>
	friend struct VariantStorer_impl;
	using StorageSize = VariantStorageSize<Types...>;
	static constexpr std::size_t Size = sizeof...(Types);
	static constexpr std::size_t Length = StorageSize::Length;
	static constexpr std::size_t Align = StorageSize::Align;
	using Storage = std::aligned_storage_t<Length, Align>;

	VariantStorage() : mIndex(Size), mStorage{} {}

protected:
	std::size_t mIndex;
	Storage mStorage;
};

template <class ...Types>
using VariantStorer = VariantStorer_impl<0, VariantStorage<Types...>, Types...>;

}

template <class ...Types>
class Variant : public detail::VariantStorage<Types...>
{
public:
	using Storer = detail::VariantStorer<Types...>;

	template <std::size_t Index>
	using IndextoType = GetTypeT<Index, Types...>;
	template <class Type>
	struct TypetoIndex
	{
		using F = FindType<Type, Types...>;
		static constexpr std::size_t Index = F::Index;
		static constexpr bool Exists = F::value;
	};

	static constexpr std::size_t Size = sizeof...(Types);

private:

	template <class Type>
	struct HasConstructibleType
	{
		static constexpr bool value = OrOperationSeq<std::is_constructible<Types, Type>::value...>::value;
	};

	template <std::size_t Index>
	struct Destroy_impl
	{
		static void apply(Variant<Types...>& v)
		{
			Storer::Destroy(IndexConstant<Index>(), v);
		}
	};

	template <std::size_t Index>
	struct Copy_impl
	{
		using Type = IndextoType<Index>;
		//コピー可である場合。
		template <class T = Type, EnableIfT<std::is_copy_constructible<T>::value> = nullptr>
		static void apply(Variant<Types...>& self, const Variant<Types...>& other)
		{
			Storer::Emplace(IndexConstant<Index>(), self, other.Get<Index>());
		}
		//コピー不可であるときは例外を投げる。
		template <class T = Type, EnableIfT<!std::is_copy_constructible<T>::value> = nullptr>
		static void apply(Variant<Types...>& self, const Variant<Types...>& other)
		{
			throw InvalidType("This type is not copy constructible");
		}
	};
	template <std::size_t Index>
	struct Move_impl
	{
		using Type = IndextoType<Index>;
		//ムーブ可である場合。
		template <class T = Type, EnableIfT<std::is_move_constructible<T>::value> = nullptr>
		static void apply(Variant<Types...>& self, Variant<Types...>&& other)
		{
			Storer::Emplace(IndexConstant<Index>(), self, std::move(other.Get<Index>()));
			//self = std::move(other.Get<Index>());
			other.Destroy();
		}
		//ムーブ不可であるときは例外を投げる。
		template <class T = Type, EnableIfT<!std::is_move_constructible<T>::value> = nullptr>
		static void apply(Variant<Types...>& self, Variant<Types...>&& other)
		{
			throw InvalidType("This type is not move constructible");
		}
	};

	template <std::size_t Index>
	struct Visit_impl
	{
		template <class F, class ...Args>
		static decltype(auto) apply(Variant<Types...>& v, F&& f, Args&& ...args)
		{
			return f(v.Get<Index>(), std::forward<Args>(args)...);
		}
		template <class F, class ...Args>
		static decltype(auto) apply(const Variant<Types...>& v, F&& f, Args&& ...args)
		{
			return f(v.Get<Index>(), std::forward<Args>(args)...);
		}
	};

public:

	template <class Type, EnableIfT<
		!std::is_same<RemoveCVRefT<Type>, Variant<Types...>>::value &&
		HasConstructibleType<Type>::value> = nullptr>
	Variant(Type&& v)
	{
		Storer::Assign(*this, std::forward<Type>(v));
	}
	template <class Type, class ...Args, EnableIfT<TypetoIndex<Type>::Exist> = nullptr>
	Variant(Args&& ...args)
	{
		Storer::template Assign<Type>(*this, std::forward<Args>(args)...);
	}
	Variant()
	{}
	Variant(const Variant<Types...>& other)
	{
		if (!other.IsEmpty())
			TabulationSwitch<Size, Copy_impl>(other.mIndex, *this, other);
		//otherが空の場合、自分も今現在空なので何もしなくていい。
	}
	Variant(Variant<Types...>&& other) noexcept
	{
		if (!other.IsEmpty())
			TabulationSwitch<Size, Move_impl>(other.mIndex, *this, std::move(other));
		//otherが空の場合、自分も今現在空なので何もしなくていい。
	}

	~Variant()
	{
		Destroy();
	}

	Variant<Types...>& operator=(const Variant<Types...>& other)
	{
		Destroy();
		if (!other.IsEmpty())
			TabulationSwitch<Size, Copy_impl>(other.mIndex, *this, other);
		return *this;
	}
	Variant<Types...>& operator=(Variant<Types...>&& other) noexcept
	{
		Destroy();
		if (!other.IsEmpty())
			TabulationSwitch<Size, Move_impl>(other.mIndex, *this, std::move(other));
		return *this;
	}

	template <class Type, EnableIfT<
		!std::is_same<RemoveCVRefT<Type>, Variant<Types...>>::value &&
		HasConstructibleType<Type>::value> = nullptr>
	Variant<Types...>& operator=(Type&& t)
	{
		Destroy();
		Storer::Assign(*this, std::forward<Type>(t));
		return *this;
	}
	template <class Type, class ...Args, EnableIfT<TypetoIndex<Type>::Exists> = nullptr>
	void Emplace(Args&& ...args)
	{
		Destroy();
		Storer::Emplace(IndexConstant<TypetoIndex<Type>::Index>(), *this, std::forward<Args>(args)...);
	}
	template <std::size_t Index, class ...Args, EnableIfT<(Index < Size)> = nullptr>
	void Emplace(Args&& ...args)
	{
		Destroy();
		Storer::Emplace(IndexConstant<Index>(), *this, std::forward<Args>(args)...);
	}

	template <class Type>
	bool Is() const
	{
		return this->mIndex == TypetoIndex<Type>::Index;
	}
	std::size_t GetIndex() const { return this->mIndex; }
	bool IsEmpty() const { return GetIndex() == Size; }

	template <std::size_t Index>
	std::enable_if_t<(Index < Size), IndextoType<Index>>& Get()
	{
		if (this->mIndex != Index) throw InvalidType("Bad Variant access");
		return Storer::Get(IndexConstant<Index>(), *this);
	}
	template <std::size_t Index>
	std::enable_if_t<(Index < Size), const IndextoType<Index>>& Get() const
	{
		if (this->mIndex != Index) throw InvalidType("Bad Variant access");
		return Storer::Get(IndexConstant<Index>(), *this);
	}

	template <class Type>
	std::enable_if_t<TypetoIndex<Type>::Exists, Type>& Get()
	{
		return Get<TypetoIndex<Type>::Index>();
	}
	template <class Type, EnableIfT<TypetoIndex<Type>::Exists> = nullptr>
	std::enable_if_t<TypetoIndex<Type>::Exists, const Type>& Get() const
	{
		return Get<TypetoIndex<Type>::Index>();
	}

	template <std::size_t Index>
	std::enable_if_t<(Index < Size), IndextoType<Index>>& Get_unsafe()
	{
		return Storer::Get(IndexConstant<Index>(), *this);
	}
	template <std::size_t Index>
	std::enable_if_t<(Index < Size), const IndextoType<Index>>& Get_unsafe() const
	{
		return Storer::Get(IndexConstant<Index>(), *this);
	}

	template <class Type>
	std::enable_if_t<TypetoIndex<Type>::Exists, Type>& Get_unsafe()
	{
		return Get<TypetoIndex<Type>::Index>();
	}
	template <class Type, EnableIfT<TypetoIndex<Type>::Exists> = nullptr>
	std::enable_if_t<TypetoIndex<Type>::Exists, const Type>& Get_unsafe() const
	{
		return Get<TypetoIndex<Type>::Index>();
	}

	template <class Func, class ...Args>
	decltype(auto) Visit(Func&& f, Args&& ...args)
	{
		if (this->mIndex == Size) throw InvalidType("Variant is empty");
		return TabulationSwitch<Size, Visit_impl>(this->mIndex, *this, std::forward<Func>(f), std::forward<Args>(args)...);
	}
	template <class Func, class ...Args>
	decltype(auto) Visit(Func&& f, Args&& ...args) const
	{
		if (this->mIndex == Size) throw InvalidType("Variant is empty");
		return TabulationSwitch<Size, Visit_impl>(this->mIndex, *this, std::forward<Func>(f), std::forward<Args>(args)...);
	}

	void Destroy()
	{
		if (this->mIndex == Size) return;
		TabulationSwitch<Size, Destroy_impl>(this->mIndex, *this);
		this->mIndex = Size;
	}
};

}

}

#endif
