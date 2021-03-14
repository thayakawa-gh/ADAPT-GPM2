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

class Any
{
	class PlaceHolder
	{
	public:
		virtual ~PlaceHolder() {}
		virtual PlaceHolder* Clone() const = 0;
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

	struct NullType {};

	Any(NullType = NullType()) : mContent(nullptr) {}

	//値を直接Holderに代入する。
	//Anyのコピー、ムーブコンストラクタが正しく機能するよう、TypeがAnyであるときだけはこの関数を呼ぶことを禁じる。
	//この仕様上、AnyにAnyを格納することは推奨しない。そんなことをする必要はどこにも感じられないが。
	//つーかこんな呼び分けしなくたってコンパイラが判断してくれるんじゃね？
	/*template <class Type,
		bool B = std::is_same<std::remove_reference_t<std::remove_const_t<Type>>, Any>::value,
		std::enable_if_t<!B, std::nullptr_t> = nullptr>
	Any(Type&& type)
		: mContent(new Holder<Type>(std::forward<Type>(type)))
	{}*/
	/*template <class Type, class ...Args>
	Any(Args&& ...args)
		: mContent(new Holder<Type>(std::forward<Args>(args)...))
	{}*/

	template <class Type, std::enable_if_t<!std::is_same<RemoveCVRefT<Type>, Any>::value, std::nullptr_t> = nullptr>
	Any(Type&& t)
		: mContent(new Holder<RemoveCVRefT<Type>>(std::forward<Type>(t)))
	{}

	Any(const Any& any)
		: mContent(any.mContent->Clone())
	{}
	Any(Any&& any) noexcept
		: mContent(std::move(any.mContent))
	{}

	Any& operator=(const Any& any)
	{
		mContent.reset(any.mContent->Clone());
		return *this;
	}
	Any& operator=(Any&& any) noexcept
	{
		mContent = std::move(any.mContent);
		return *this;
	}

	template <class Type>
	Any& operator=(Type&& v)
	{
		mContent = std::make_unique<Holder<RemoveCVRefT<Type>>>(std::forward<Type>(v));
		return *this;
	}

	template <class Type, class ...Args>
	void Emplace(Args&& ...args)
	{
		mContent = std::make_unique<Holder<Type>>(std::forward<Args>(args)...);
	}

	template <class Type>
	Type& Get() &
	{
		return static_cast<Holder<Type>&>(*mContent).mValue;
	}
	template <class Type>
	const Type& Get() const &
	{
		return static_cast<const Holder<Type>&>(*mContent).mValue;
	}
	template <class Type>
	Type&& Get() &&
	{
		return static_cast<Holder<Type>&&>(*mContent).Value;
	}

	bool IsEmpty() const { return !mContent; }

	template <class Type>
	bool Is() const
	{
		return dynamic_cast<const Holder<Type>*>(mContent.get()) != nullptr;
	}

private:
	std::unique_ptr<PlaceHolder> mContent;
};

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
			static PlaceHolder* f(const Holder<Type>& h) { throw InvalidType("Assigned class is not copyable."); }
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

	template <class Type>
	Type& Get() &
	{
		return static_cast<Holder<Type>&>(*mContent).mValue;
	}
	template <class Type>
	const Type& Get() const &
	{
		return static_cast<const Holder<Type>&>(*mContent).mValue;
	}
	template <class Type>
	Type&& Get()&&
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

	bool IsEmpty() const { return !mContent; }

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
	bool Is() const { return Base::Is<Type&&>(); }
};

}

}

#endif