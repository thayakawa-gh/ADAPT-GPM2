#ifndef CUF_ANYTYPE_H
#define CUF_ANYTYPE_H

#include <memory>
#include <typeindex>
#include <ADAPT/CUF/Template.h>
#include <ADAPT/CUF/Exception.h>

namespace adapt
{

inline namespace cuf
{

namespace detail
{

//AllowBigObjがfalseの場合、StrgSizeより大きなオブジェクトは格納できなくなる。
template <size_t StrgSize, bool AllowBigObj>
class Any_impl
{
	template <class T>
	static constexpr bool IsAny()
	{
		return std::is_same_v<std::decay_t<T>, Any_impl>;
	}

	template <class T>
	static constexpr bool IsSmall()
	{
		return
			sizeof(T) <= StrgSize &&
			std::is_nothrow_move_constructible_v<T>;
	}

	struct RTFuncs
	{
		template <class T, bool Small>
		static void Copy(Any_impl& to, const Any_impl& from)
		{
			if constexpr (Small)
			{
				auto& t = reinterpret_cast<T&>(to.mStorage.mSmall.mData);
				new (&t) T(from.Get_unsafe<T>());
			}
			else if constexpr (AllowBigObj)
			{
				to.mStorage.mBig.mPtr = new T(from.Get_unsafe<T>());
			}
			else
				static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
		}
		template <class T, bool Small>
		static void Move(Any_impl& to, Any_impl&& from) noexcept
		{
			if constexpr (Small)
			{
				auto& t = reinterpret_cast<T&>(to.mStorage.mSmall.mData);
				auto& f = reinterpret_cast<T&>(from.mStorage.mSmall.mData);
				new (&t) T(std::move(f));
				f.~T();
			}
			else if constexpr (AllowBigObj)
			{
				to.mStorage.mBig.mPtr = from.mStorage.mBig.mPtr;
				from.mStorage.mBig.mPtr = nullptr;
			}
			else
				static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
		}
		template <class T, bool Small>
		static void Destroy(Any_impl& to) noexcept
		{
			if constexpr (Small)
			{
				auto& t = reinterpret_cast<T&>(to.mStorage.mSmall.mData);
				t.~T();
				to.mStorage.mSmall = SmallStorage{};
			}
			else if constexpr (AllowBigObj)
			{
				auto* t = reinterpret_cast<T*>(to.mStorage.mBig.mPtr);
				delete t;
				to.mStorage.mBig = BigStorage{};
			}
			else
				static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
		}
		template <class T>
		static const std::type_info& TypeInfo()
		{
			return typeid(T);
		}
		using CopyF = void(Any_impl&, const Any_impl&);
		using MoveF = void(Any_impl&, Any_impl&&);
		using DestroyF = void(Any_impl&);
		using TypeInfoF = const std::type_info& (void);

		CopyF* mCopy = nullptr;
		MoveF* mMove = nullptr;
		DestroyF* mDestroy = nullptr;
		TypeInfoF* mTypeInfo = nullptr;
	};
	template <class T, bool Small = IsSmall<T>()>
	inline static constexpr RTFuncs RTFuncs_value =
	{ &RTFuncs::template Copy<T, Small>, &RTFuncs::template Move<T, Small>,
	  &RTFuncs::template Destroy<T, Small>, &RTFuncs::template TypeInfo<T> };

public:

	struct NullType {};

	Any_impl() : mRTFuncs(nullptr) {}
	template <class T, std::enable_if_t<!IsAny<T>(), std::nullptr_t> = nullptr>
	Any_impl(T&& v)
		: mRTFuncs(nullptr)
	{
		Emplace_impl<std::decay_t<T>>(std::forward<T>(v));
	}
	Any_impl(const Any_impl& a)
		: mRTFuncs(nullptr)
	{
		if (!a.IsEmpty()) Copy(a);
	}
	Any_impl(Any_impl&& a) noexcept
		: mRTFuncs(nullptr)
	{
		if (!a.IsEmpty()) Move(std::move(a));
	}

	template <class T, std::enable_if_t<!IsAny<T>(), std::nullptr_t> = nullptr>
	Any_impl& operator=(T&& v)
	{
		Emplace<std::decay_t<T>>(std::forward<T>(v));
		return *this;
	}

	Any_impl& operator=(const Any_impl& a)
	{
		if (!IsEmpty()) Destroy();
		if (!a.IsEmpty()) Copy(a);
		return *this;
	}
	Any_impl& operator=(Any_impl&& a) noexcept
	{
		if (!IsEmpty()) Destroy();
		if (!a.IsEmpty()) Move(std::move(a));
		return *this;
	}
	~Any_impl()
	{
		if (!IsEmpty()) Destroy();
	}

	template <class T, class ...Args>
	void Emplace(Args&& ...args)
	{
		if (!IsEmpty()) Destroy();
		Emplace_impl<T>(std::forward<Args>(args)...);
	}

	bool IsEmpty() const { return mRTFuncs == nullptr; }
	operator bool() const { return !IsEmpty(); }
	template <class T>
	bool Is() const
	{
		return !IsEmpty() && typeid(T) == TypeInfo();
	}
	const std::type_info& GetType() const
	{
		if (IsEmpty()) return typeid(NullType);
		return TypeInfo();
	}

	template <class T>
	const T& Get() const&
	{
		if (!Is<T>()) throw InvalidType("bad cast of Any");
		return Get_unsafe<T>();
	}
	template <class T>
	T& Get()&
	{
		if (!Is<T>()) throw InvalidType("bad cast of Any");
		return Get_unsafe<T>();
	}
	template <class T>
	T&& Get()&&
	{
		if (!Is<T>()) throw InvalidType("bad cast of Any");
		return Get_unsafe<T>();
	}

	template <class T>
	const T& Get_unsafe() const&
	{
		assert(!IsEmpty());
		if constexpr (IsSmall<T>())
			return reinterpret_cast<const T&>(mStorage.mSmall.mData);
		else if constexpr (AllowBigObj)
			return *reinterpret_cast<const T*>(mStorage.mBig.mPtr);
		else
			static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
	}
	template <class T>
	T& Get_unsafe()&
	{
		assert(!IsEmpty());
		if constexpr (IsSmall<T>())
			return reinterpret_cast<T&>(mStorage.mSmall.mData);
		else if constexpr (AllowBigObj)
			return *reinterpret_cast<T*>(mStorage.mBig.mPtr);
		else
			static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
	}
	template <class T>
	T&& Get_unsafe()&&
	{
		assert(!IsEmpty());
		if constexpr (IsSmall<T>())
			return std::move(reinterpret_cast<T&>(mStorage.mStorage.mSmall.mData));
		else if constexpr (AllowBigObj)
			return std::move(*reinterpret_cast<T*>(mStorage.mBig.mPtr));
		else
			static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
	}

private:

	void Copy(const Any_impl& from)
	{
		assert(IsEmpty());
		mRTFuncs = from.mRTFuncs;
		mRTFuncs->mCopy(*this, from);
	}
	void Move(Any_impl&& from) noexcept
	{
		assert(IsEmpty());
		mRTFuncs = from.mRTFuncs;
		mRTFuncs->mMove(*this, std::move(from));
		from.mRTFuncs = nullptr;
	}
	void Destroy() noexcept
	{
		assert(!IsEmpty());
		auto* d = mRTFuncs->mDestroy;
		mRTFuncs = nullptr;
		d(*this);
	}
	const std::type_info& TypeInfo() const
	{
		assert(!IsEmpty());
		return mRTFuncs->mTypeInfo();
	}

	//Tはdecayされているものとする。
	template <class T, class ...Args>
	void Emplace_impl(Args&& ...args)
	{
		assert(IsEmpty());
		if constexpr (IsSmall<T>())
		{
			//small
			auto* s = reinterpret_cast<T*>(&mStorage.mSmall.mData);
			new (s) T(std::forward<Args>(args)...);
			mRTFuncs = &RTFuncs_value<T>;
		}
		else if constexpr (AllowBigObj)
		{
			//big
			mStorage.mBig.mPtr = new T(std::forward<Args>(args)...);
			mRTFuncs = &RTFuncs_value<T>;
		}
		else
			static_assert([]() { return false; }, "size of the template argument is greater than Any's storage.");
	}

	struct BigStorage
	{
		char Padding[StrgSize - sizeof(void*)];
		void* mPtr;
	};
	struct SmallStorage
	{
		std::aligned_storage_t<StrgSize, alignof(std::max_align_t)> mData;
	};
	template <bool, class = void>
	union Storage
	{
		BigStorage mBig;
		SmallStorage mSmall;
	};
	template <class Dummy>
	union Storage<false, Dummy>
	{
		SmallStorage mSmall;
	};

	Storage<AllowBigObj> mStorage;
	const RTFuncs* mRTFuncs;
};

}

using Any = detail::Any_impl<24, true>;
template <size_t Size>
using StaticAny = detail::Any_impl<Size, false>;

//shared_ptr+anyみたいなもの。機能的にはshared_ptr<void>と大差ない気がする。
//任意の型の値を格納することができ、なおかつreference countを持ち共有することができる。
//通常のコピーはshared_ptrと同じようにshallow copyするだけで、
//完全に複製するにはClone関数を呼ぶ必要がある。
//std::shared_ptrはポインタx2の大きさを持つため、ArbValueに格納することができない。
//ので、weak_ptrに相当する機能を排しポインタx1の大きさにしたAnyを用意することにした。
//循環参照対策をしていないので注意。
class ShareableAny
{
	class PlaceHolder
	{
	public:
		PlaceHolder() : mCount(1) {}
		virtual ~PlaceHolder() {}
		virtual PlaceHolder* Clone() const = 0;

		int Increase() { return ++mCount; }
		int Decrease() { return --mCount; }
		int GetCount() const { return mCount; }
	private:
		int mCount;
	};

	template <class Type>
	class Holder : public PlaceHolder
	{
	public:
		template <class ...Args>
		Holder(Args&& ...args)
			: mValue(std::forward<Args>(args)...)
		{}
	private:
		template <class Dummy = void, bool B = std::is_copy_constructible<Type>::value>
		struct Clone_impl;
		template <class Dummy, bool B>
		struct Clone_impl
		{
			static PlaceHolder* f(const Holder<Type>& h) { return new Holder<Type>(h.mValue); }
		};
		template <class Dummy>
		struct Clone_impl<Dummy, false>
		{
			static PlaceHolder* f(const Holder<Type>&) { throw InvalidType("Assigned class is not copyable."); }
		};
	public:
		virtual PlaceHolder* Clone() const override
		{
			return Clone_impl<>::f(*this);
		}

		Type mValue;
	};

public:

	ShareableAny(Any::NullType = Any::NullType()) : mContent(nullptr) {}

	template <class Type,
			std::enable_if_t<!std::is_same<RemoveCVRefT<Type>, ShareableAny>::value, std::nullptr_t> = nullptr>
	ShareableAny(Type&& type)
		: mContent(new Holder<Type>(std::forward<Type>(type)))
	{}
	/*template <class Type, class ...Args>
	ShareableAny(Args&& ...args)
		: mContent(new Holder<Type>(std::forward<Args>(args)...))
	{}*/

	//ShareableAnyのコピーはShallowCopyである。
	ShareableAny(const ShareableAny& any)
		: mContent(any.mContent)
	{
		mContent->Increase();
	}
	ShareableAny(ShareableAny&& any) noexcept
		: mContent(any.mContent)
	{
		any.mContent = nullptr;
	}

	ShareableAny& operator=(const ShareableAny& any)
	{
		Delete();
		mContent = any.mContent;
		mContent->Increase();
		return *this;
	}
	ShareableAny& operator=(ShareableAny&& any) noexcept
	{
		Delete();
		mContent = any.mContent;
		any.mContent = nullptr;
		return *this;
	}

	~ShareableAny()
	{
		Delete();
	}

	template <class Type, class ...Args>
	void Emplace(Args&& ...args)
	{
		Delete();
		mContent = new Holder<Type>(std::forward<Args>(args)...);
	}

	bool IsEmpty() const { return !mContent; }
	template <class T>
	bool Is() const
	{
		return dynamic_cast<const Holder<T>&>(*mContent) != nullptr;
	}

	template <class Type>
	const Type& Get() const&
	{
		if (!Is<Type>()) throw InvalidType("bad ShareableAny cast");
		return Get_unsafe<Type>();
	}
	template <class Type>
	Type& Get() &
	{
		if (!Is<Type>()) throw InvalidType("bad ShareableAny cast");
		return Get_unsafe<Type>();
	}
	template <class Type>
	Type&& Get() &&
	{
		if (!Is<Type>()) throw InvalidType("bad ShareableAny cast");
		return Get_unsafe<Type>();
	}
	template <class Type>
	Type& Get_unsafe() &
	{
		return static_cast<Holder<Type>&>(*mContent).mValue;
	}
	template <class Type>
	const Type& Get_unsafe() const &
	{
		return static_cast<const Holder<Type>&>(*mContent).mValue;
	}
	template <class Type>
	Type&& Get_unsafe()&&
	{
		return static_cast<const Holder<Type>&>(*mContent).mValue;
	}

	//ShareableAny Clone() const { return ShareableAny(mContent->Clone()); }

	void Delete()
	{
		if (mContent)
		{
			mContent->Decrease();
			if (mContent->GetCount() == 0) delete mContent;
			mContent = nullptr;
		}
	}
	int GetCount() const { return mContent->GetCount(); }

private:
	PlaceHolder* mContent;
};

class AnyURef
{
protected:

	class HolderBase
	{
	public:
		virtual void CopyTo(void* b) const = 0;
		virtual std::type_index GetTypeIndex() const = 0;
	};

	template <class T>
	class Holder : public HolderBase
	{
	public:
		Holder() = default;
		Holder(T v) : mValue(static_cast<T>(v)) {}
		Holder(const Holder& h) : mValue(static_cast<T>(h.mValue)) {}
		virtual void CopyTo(void* ptr) const
		{
			new (ptr) Holder<T>(*this);
		}
		virtual std::type_index GetTypeIndex() const { return typeid(T); }
		T mValue;
	};

	template <class Type>
	void Construct(Type&& v)
	{
		static_assert(sizeof(Holder<Type&&>) >= sizeof(mStorage), "the size of storage is insufficient.");
		new ((void*)&mStorage) Holder<Type&&>(static_cast<Type&&>(v));
	}

	template <class Type>
	using NotAnyURefOrNull = std::enable_if_t<!std::is_same_v<RemoveCVRefT<Type>, AnyURef> &&
		!std::is_same_v<RemoveCVRefT<Type>, Any::NullType>, std::nullptr_t>;

public:

	AnyURef(Any::NullType = Any::NullType())
	{
		//こちらだとGCC10.1以降、-std=c++17を有効にしたとき何故かコンパイルエラーになる。
		//new ((void*)&mStorage) Holder<NullType>;

		//一時オブジェクトなので寿命が尽きるが、中身にアクセスすることはないので一時的な対処として。
		Construct(Any::NullType());
	}

	template <class Type, NotAnyURefOrNull<Type> = nullptr>
	AnyURef(Type&& v)
	{
		Construct(std::forward<Type>(v));
	}
	AnyURef(const AnyURef& a)
	{
		reinterpret_cast<const HolderBase*>(&a.mStorage)->CopyTo(&mStorage);
	}
	template <class Type, NotAnyURefOrNull<Type> = nullptr>
	AnyURef& operator=(Type&& v)
	{
		Construct(std::forward<Type>(v));
		return *this;
	}
	AnyURef& operator=(const AnyURef& a)
	{
		//copy
		const HolderBase* q = reinterpret_cast<const HolderBase*>(&a.mStorage);
		q->CopyTo(&mStorage);
		return *this;
	}

	template <class Type>
	Type Get() const
	{
		if (!Is<Type>()) throw InvalidType("bad AnyRef cast");
		return Get_unsafe<Type>();
	}
	template <class Type>
	Type Get_unsafe() const
	{
		const Holder<Type>* p = GetHolder_unsafe<Type>();
		return static_cast<Type>(p->mValue);
	}

	template <class Type>
	bool Is() const
	{
		return GetHolder<Type>() != nullptr;
	}

	bool IsEmpty() const
	{
		return Is<Any::NullType&&>();
	}

	std::type_index GetTypeIndex() const
	{
		return GetHolderBase()->GetTypeIndex();
	}

private:

	template <class Type>
	const Holder<Type>* GetHolder_unsafe() const
	{
		const HolderBase* p = reinterpret_cast<const HolderBase*>(&mStorage);
		return static_cast<const Holder<Type>*>(p);
	}
	template <class Type>
	const Holder<Type>* GetHolder() const
	{
		const HolderBase* p = reinterpret_cast<const HolderBase*>(&mStorage);
		return dynamic_cast<const Holder<Type>*>(p);
	}
	const HolderBase* GetHolderBase() const
	{
		return reinterpret_cast<const HolderBase*>(&mStorage);
	}

	std::aligned_storage_t<16> mStorage;
};

class AnyRef : public AnyURef
{
	using Base = AnyURef;

	template <class Type>
	using NotURefOrNullOrConst = std::enable_if_t<!std::is_base_of_v<AnyURef, Type> &&
		!std::is_const_v<Type> &&
		!std::is_same_v<Type, Any::NullType>, std::nullptr_t>;

public:

	AnyRef(Any::NullType = Any::NullType()) : AnyURef() {}

	template <class Type, NotURefOrNullOrConst<Type> = nullptr>
	AnyRef(Type& v) : Base(v)
	{}
	AnyRef(const AnyRef& a) : Base(static_cast<const AnyURef&>(a))
	{}

	AnyRef& operator=(const AnyRef& a)
	{
		Base::operator=(static_cast<const AnyURef&>(a));
		return *this;
	}
	template <class Type, NotURefOrNullOrConst<Type> = nullptr>
	AnyRef& operator=(Type& a)
	{
		Base::operator=(a);
		return *this;
	}

	template <class Type>
	Type& Get() const { return Base::Get<Type&>(); }
	template <class Type>
	Type& Get_unsafe() const { return Base::Get_unsafe<Type&>(); }
	template <class Type>
	bool Is() const { return Base::Is<Type&>(); }
};
class AnyCRef : public AnyURef
{
	using Base = AnyURef;

	template <class Type>
	using NotAnyURefOrNull = std::enable_if_t<!std::is_base_of_v<AnyURef, Type> &&
		!std::is_same_v<Type, Any::NullType>, std::nullptr_t>;

public:

	AnyCRef(Any::NullType = Any::NullType{}) : AnyURef() {}

	template <class Type, NotAnyURefOrNull<Type> = nullptr>
	AnyCRef(const Type& v) : Base(v)
	{}
	AnyCRef(const AnyCRef& a) : Base(static_cast<const AnyURef&>(a))
	{}

	AnyCRef& operator=(const AnyCRef& a)
	{
		Base::operator=(static_cast<const AnyURef&>(a));
		return *this;
	}
	template <class Type, NotAnyURefOrNull<Type> = nullptr>
	AnyCRef& operator=(const Type& a)
	{
		Base::operator=(a);
		return *this;
	}

	template <class Type>
	const Type& Get() const { return Base::Get<const Type&>(); }
	template <class Type>
	const Type& Get_unsafe() const { return Base::Get_unsafe<const Type&>(); }
	template <class Type>
	bool Is() const { return Base::Is<const Type&>(); }
};
class AnyRRef : public AnyURef
{
	using Base = AnyURef;

	template <class Type>
	using NotURefNullLValue = std::enable_if_t<!std::is_base_of_v<AnyURef, Type>&&
		std::is_rvalue_reference_v<Type&&> &&
		!std::is_same_v<Type, Any::NullType>, std::nullptr_t>;
public:

	AnyRRef(Any::NullType = Any::NullType()) : AnyURef() {}

	template <class Type, NotURefNullLValue<Type> = nullptr>
	AnyRRef(Type&& v) : Base(std::move(v))
	{}
	AnyRRef(const AnyRRef& a) : Base(static_cast<const AnyURef&>(a))
	{}

	AnyRRef& operator=(const AnyRRef& a)
	{
		Base::operator=(static_cast<const AnyRRef&>(a));
		return *this;
	}
	template <class Type, NotURefNullLValue<Type> = nullptr>
	AnyRRef& operator=(Type&& a)
	{
		Base::operator=(a);
		return *this;
	}

	template <class Type>
	Type&& Get() const { return Base::Get<Type&&>(); }
	template <class Type>
	Type&& Get_unsafe() const { return Base::Get_unsafe<Type&&>(); }
	template <class Type>
	bool Is() const { return Base::Is<Type&&>(); }
};

}

}

#endif