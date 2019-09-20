#pragma once

template <typename ReturnType, typename ...Args>
struct Delegate
{
	template <typename T, typename ReturnType(T::* fn)(Args...)>
	static Delegate make(T* target)
	{
		Delegate d;
		d.target = target;
		d.function = make_stub<T, fn>;
		return d;
	}

	template <typename ReturnType(*fn)(Args...)>
	static Delegate make()
	{
		Delegate d;
		d.function = make_stub<fn>;
		return d;
	}

	ReturnType invoke(Args&& ...args) const
	{
		return function(target, args...);
	}

	bool isValid() const { return function != nullptr; }

private:
	template <typename T, typename ReturnType(T::* fn)(Args...)>
	static ReturnType make_stub(void* target, Args ...args)
	{
		return (static_cast<T*>(target)->*fn)(std::forward<Args>(args)...);
	}
	template <typename ReturnType(*fn)(Args...)>
	static ReturnType make_stub(void* target, Args ...args)
	{
		return (*fn)(std::forward<Args>(args)...);
	}

	typedef ReturnType(*Func)(void*, Args...);
	void* target;
	Func function;
};