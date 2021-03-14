#ifndef CUF_STATIC_ANY_H
#define CUF_STATIC_ANY_H

#include <type_traits>
#include <ADAPT/CUF/Template.h>
#include <ADAPT/CUF/Exception.h>

namespace adapt
{

inline namespace cuf
{

template <size_t Size, size_t Align>
class StaticAny;

namespace detail
{

template <class Type>
class IsStaticAny
{
private:
	template <auto N, auto M>
	static constexpr std::true_type check(const StaticAny<N, M>*);
	static constexpr std::false_type check(const void*);

	static const Type* d;
public:
	static constexpr bool value = decltype(check(d))::value;
};

}

template <size_t Capacity_, size_t Align_ = 8>
class StaticAny
{
	static constexpr size_t Capacity = Capacity_;
	static constexpr size_t Align = Align_;
	template <size_t Cap, size_t Ali>
	friend class StaticAny;

	class NullType
	{
		NullType(std::nullptr_t n) : mNullPtr(n) {}
		std::nullptr_t mNullPtr;
	};

	class HolderBase
	{
	public:
		virtual ~HolderBase() {}
		virtual void CopyTo(HolderBase*) const = 0;
		virtual void MoveTo(HolderBase*) = 0;
	};
	template <class Type>
	class Holder : public HolderBase
	{
	public:
		template <class ...Args>
		Holder(Args&& ...args)
			: mValue(std::forward<Args>(args)...)
		{}
	private:
		template <class Dummy = void, bool B = std::is_copy_constructible_v<Type>>
		struct CopyTo_impl
		{
			static void Apply(const Holder<Type>* self, Holder<Type>* to) { new (to) Holder<Type>(*self); }
		};
		template <class Dummy>
		struct CopyTo_impl<Dummy, false>
		{
			static void Apply(const Holder<Type>* self, Holder<Type>* to) { throw InvalidType("Assigned class is not copyable"); }
		};
		template <class Dummy = void, bool B = std::is_copy_constructible_v<Type> || std::is_move_constructible_v<Type>>
		struct MoveTo_impl
		{
			static void Apply(Holder<Type>* self, Holder<Type>* to) { new (to) Holder<Type>(std::move(*self)); }
		};
		template <class Dummy>
		struct MoveTo_impl<Dummy, false>
		{
			static void Apply(const Holder<Type>* self, Holder<Type>* to) { throw InvalidType("Assigned class is not copyable"); }
		};
	public:
		virtual void CopyTo(HolderBase* to) const override
		{
			Holder<Type>* p = static_cast<Holder<Type>*>(to);
			CopyTo_impl<>::Apply(this, p);
		}
		virtual void MoveTo(HolderBase* to) override
		{
			Holder<Type>* p = static_cast<Holder<Type>*>(to);
			MoveTo_impl<>::Apply(this, p);
		}

		Type mValue;
	};
	template <class Type>
	void Construct(Type&& arg)
	{
		using Type_ = std::decay_t<Type>;
		Holder<Type>* p = reinterpret_cast<Holder<Type>*>(&mStorage);
		new (p) Holder<Type>(std::forward<Type>(arg));
	}
	template <class Type, class ...Args>
	void Construct(Args&& ...args)
	{
		using Type_ = std::decay_t<Type>;
		Holder<Type>* p = reinterpret_cast<Holder<Type>*>(&mStorage);
		new (p) Holder<Type>(std::forward<Args>(args)...);
	}

public:

	StaticAny() : mStorage{}
	{
		Construct(NullType(nullptr));
	}
	template <class Type,
		bool B = (Capacity >= sizeof(std::decay_t<Type>) && !detail::IsStaticAny<std::decay_t<Type>>::value),
		std::enable_if_t<B, std::nullptr_t> = nullptr>
		StaticAny(Type&& arg)
	{
		Construct(std::forward<Type>(arg));
	}
	template <size_t Cap, size_t Ali, std::enable_if_t<Capacity >= Cap, std::nullptr_t> = nullptr>
	StaticAny(const StaticAny<Cap, Ali>& other)
	{
		other.GetHolderBase()->CopyTo(GetHolderBase());
	}
	template <size_t Cap, size_t Ali, std::enable_if_t<Capacity >= Cap, std::nullptr_t> = nullptr>
	StaticAny(StaticAny<Cap, Ali>&& other) noexcept
	{
		other.GetHolderBase()->MoveTo(GetHolderBase());
	}

	template <class Type,
		bool B = (Capacity >= sizeof(std::decay_t<Type>) && !detail::IsStaticAny<std::decay_t<Type>>::value),
		std::enable_if_t<B, std::nullptr_t> = nullptr>
		StaticAny& operator=(Type&& arg)
	{
		GetHolderBase()->~HolderBase();
		Construct(std::forward<Type>(arg));
		return *this;
	}
	template <size_t Cap, size_t Ali, std::enable_if_t<Capacity >= Cap, std::nullptr_t> = nullptr>
	StaticAny& operator=(const StaticAny<Cap, Ali>& other)
	{
		GetHolderBase()->~HolderBase();
		other.GetHolderBase()->CopyTo(GetHolder());
		return *this;
	}
	template <size_t Cap, size_t Ali, std::enable_if_t<Capacity >= Cap, std::nullptr_t> = nullptr>
	StaticAny& operator=(StaticAny<Cap, Ali>&& other) noexcept
	{
		GetHolderBase()->~HolderBase();
		other.GetHolderBase()->MoveTo(GetHolder());
		return *this;
	}
	~StaticAny()
	{
		GetHolderBase()->~HolderBase();
	}

	void Destroy()
	{
		GetHolderBase()->~HolderBase();
		Construct(NullType(nullptr));
	}

	template <class Type, class ...Args>
	std::enable_if_t<(Capacity >= sizeof(Type))> Emplace(Args&& ...args)
	{
		GetHolderBase()->~HolderBase();
		Construct<Type>(std::forward<Args>(args)...);
	}

	template <class Type>
	Type& Get()&
	{
		Holder<Type>* p = GetHolder_unsafe<Type>();
		assert(p != nullptr);
		return p->mValue;
	}
	template <class Type>
	const Type& Get() const&
	{
		const Holder<Type>* p = GetHolder_unsafe<Type>();
		assert(p != nullptr);
		return p->mValue;
	}
	template <class Type>
	Type&& Get()&&
	{
		Holder<Type>* p = GetHolder_unsafe<Type>();
		assert(p != nullptr);
		return std::move(p->mValue);
	}

	template <class Type>
	bool Is() const
	{
		const Holder<Type>* p = GetHolder<Type>();
		return p != nullptr;
	}

	template <class Type>
	bool IsEmpty() const { return Is<NullType>(); }

private:

	template <class Type>
	Holder<Type>* GetHolder_unsafe() { return static_cast<Holder<Type>*>(GetHolderBase()); }
	template <class Type>
	const Holder<Type>* GetHolder_unsafe() const { return static_cast<const Holder<Type>*>(GetHolderBase()); }
	template <class Type>
	Holder<Type>* GetHolder() { return dynamic_cast<Holder<Type>*>(GetHolderBase()); }
	template <class Type>
	const Holder<Type>* GetHolder() const { return dynamic_cast<const Holder<Type>*>(GetHolderBase()); }
	HolderBase* GetHolderBase() { return reinterpret_cast<HolderBase*>(&mStorage); }
	const HolderBase* GetHolderBase() const { return reinterpret_cast<const HolderBase*>(&mStorage); }

	std::aligned_storage_t<Capacity + sizeof(void*), Align> mStorage;
};

}

}

#endif