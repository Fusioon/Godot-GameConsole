#pragma once

enum class ECVarType : int8_t
{
	Invalid = 0,
	Bool,
	Int32,
	Float,
	String,
	Int64,
	Command
};

enum class ECVarFlags : int32_t
{
	Null = 0x00000000,
	Save = 0x00000001,
	Cheat = 0x00000002,
};

enum class ELogType : int8_t
{
	Null,
	Success,
	Warning,
	Error,
	Exception,
	MAX
};

#include <type_traits>

template <typename T>
struct EnumUtils {
	//static_assert(std::is_enum<T>::value);
	using underlying_type = typename std::underlying_type<T>::type;

	static underlying_type to_underlying(T value) {
		return static_cast<underlying_type>(value);
	}
	static T to_enum(underlying_type value) {
		return static_cast<T>(value);
	}
};

struct IConsoleArgumentAutoComplete {
	virtual ~IConsoleArgumentAutoComplete() {}
	
	virtual size_t count() = 0;
	virtual bool get_value(const godot::String& input, godot::String& result) const = 0;
};

struct CmdExecArgs
{
	CmdExecArgs(const godot::String& line, const godot::String* args, const size_t count) :
		line(line),
		m_args(args),
		count(count)
	{ }

	const size_t count;
	const godot::String& line;

	const godot::String& arg(size_t i) const { return m_args[i]; }
	const godot::String& operator[](size_t i) const { return arg(i); }
	godot::String line_remove_command() const {
		godot::String cmd_name = arg(0);
		int i = cmd_name.length();
		while (i < line.length() && is_whitespace(line[i])) {
			++i;
		}
		if (i < line.length()) {
			return line.substr(i, line.length());
		}
		return godot::String();
	}
private:
	bool is_whitespace(wchar_t c) const { return c == '\t' || c == ' '; }
	const godot::String* m_args;
};

struct ICVar
{
	virtual ~ICVar() {}
	virtual const godot::String& GetName() const = 0;
	virtual const godot::String& GetHelp() const = 0;
	virtual ECVarFlags GetFlags() const = 0;
	virtual ECVarType GetType() const = 0;

	virtual bool GetValueBool() const = 0;
	virtual int32_t GetValueInt32() const = 0;
	virtual int64_t GetValueInt64() const = 0;
	virtual float GetValueFloat() const = 0;
	virtual godot::String GetValueString() const = 0;

	virtual bool Execute(const CmdExecArgs& cmdLine) = 0;
};

struct IConsoleUI
{
	virtual void Open() = 0;
	virtual void Close() = 0;
	virtual void PrintLine(const godot::String& text, ELogType log_type) = 0;
	virtual void Clear() = 0;
};


template <typename T>
struct ConPropMethods
{

	ConPropMethods() : getter(nullptr), setter(nullptr), target(nullptr) {}

	typedef T(*Get)(void*);
	typedef void(*Set)(void*, const T&);
	Get getter;
	Set setter;
	void* target;

	template <typename T(*Getter)()>
	static T getterNoTarget(void* target)
	{
		return (*Getter)();
	}
	template <typename void(*Setter)(T)>
	static void setterNoTarget(void* target, const T& var)
	{
		(*Setter)(T(var));
	}
	template <typename void(*Setter)(const T&)>
	static void setterNoTarget(void* target, const T& var)
	{
		(*Setter)(var);
	}

	template <typename Object, typename T(Object::* Getter)() const>
	static T getterWithTarget(void* target)
	{
		return (static_cast<Object*>(target)->*Getter)();
	}
	template <typename Object, typename T(Object::* Getter)()>
	static T getterWithTarget(void* target)
	{
		return (static_cast<Object*>(target)->*Getter)();
	}
	template <typename Object, typename void(Object::* Setter)(T)>
	static void setterWithTarget(void* target, const T& var)
	{
		(static_cast<Object*>(target)->*Setter)(T(var));
	}
	template <typename Object, typename void(Object::* Setter)(const T&)>
	static void setterWithTarget(void* target, const T& var)
	{
		(static_cast<Object*>(target)->*Setter)(var);
	}
};

template <typename T, T(*Getter)(), void(*Setter)(T)>
ConPropMethods<T> make_property()
{
	ConPropMethods<T> methods;
	methods.getter = &ConPropMethods<T>::template getterNoTarget<Getter>;
	methods.setter = &ConPropMethods<T>::template setterNoTarget<Setter>;
	return methods;
}

template <typename bool(*Getter)(), typename void(*Setter)(bool)>
auto make_property()
{
	return make_property<bool, Getter, Setter>();
}
template <typename int32_t(*Getter)(), typename void(*Setter)(int32_t)>
auto make_property()
{
	return make_property<int32_t, Getter, Setter>();
}
template <typename int64_t(*Getter)(), typename void(*Setter)(int64_t)>
auto make_property()
{
	return make_property<int64_t, Getter, Setter>();
}
template <typename float(*Getter)(), typename void(*Setter)(float)>
auto make_property()
{
	return make_property<float, Getter, Setter>();	
	//return make_property<float, Getter, Setter>();
}
template <typename godot::String(*Getter)(), typename void(*Setter)(const godot::String&)>
auto make_property()
{
	ConPropMethods<godot::String> methods;
	methods.getter = &ConPropMethods<godot::String>::getterNoTarget<Getter>;
	methods.setter = &ConPropMethods<godot::String>::setterNoTarget<Setter>;
	return methods;
}

template <typename T, typename Obj, T(Obj::*Getter)(), void(Obj::*Setter)(T)>
ConPropMethods<T> make_property(Obj* target)
{
	ConPropMethods<T> methods;
	methods.target = target;
	methods.getter = &ConPropMethods<T>::template getterWithTarget<Obj, Getter>;
	methods.setter = &ConPropMethods<T>::template setterWithTarget<Obj, Setter>;
	return methods;
}
template <typename Obj, typename bool(Obj::*Getter)(), typename void(Obj::*Setter)(bool)>
auto make_property(Obj* target)
{
	return make_property<bool, Obj, Getter, Setter>(target);
}
template <typename Obj, typename bool(Obj::* Getter)() const, typename void(Obj::* Setter)(bool)>
auto make_property(Obj* target)
{
	return make_property<bool, Obj, Getter, Setter>(target);
}
template <typename Obj, typename int32_t(Obj::* Getter)(), typename void(Obj::* Setter)(int32_t)>
auto make_property(Obj* target)
{
	return make_property<int32_t, Obj, Getter, Setter>(target);
}
template <typename Obj, typename int32_t(Obj::* Getter)() const, typename void(Obj::* Setter)(int32_t)>
auto make_property(Obj* target)
{
	return make_property<int32_t, Obj, Getter, Setter>(target);
}
template <typename Obj, typename int64_t(Obj::* Getter)(), typename void(Obj::* Setter)(int64_t)>
auto make_property(Obj* target)
{
	return make_property<int64_t, Obj, Getter, Setter>(target);
}
template <typename Obj, typename int64_t(Obj::* Getter)() const, typename void(Obj::* Setter)(int64_t)>
auto make_property(Obj* target)
{
	return make_property<int64_t, Obj, Getter, Setter>(target);
}
template <typename Obj, typename float(Obj::* Getter)(), typename void(Obj::* Setter)(float)>
auto make_property(Obj* target)
{
	return make_property<float, Obj, Getter, Setter>(target);
}
template <typename Obj, typename float(Obj::* Getter)() const, typename void(Obj::* Setter)(float)>
auto make_property(Obj* target)
{
	return make_property<float, Obj, Getter, Setter>(target);
}
template <typename Obj, typename godot::String(Obj::* Getter)(), typename void(Obj::* Setter)(const godot::String&)>
auto make_property(Obj* target)
{
	using T = godot::String;
	ConPropMethods<T> methods;
	methods.target = target;
	methods.getter = &ConPropMethods<T>::getterWithTarget<Obj, Getter>;
	methods.setter = &ConPropMethods<T>::setterWithTarget<Obj, Setter>;
	return methods;
}
template <typename Obj, typename godot::String(Obj::* Getter)() const, typename void(Obj::* Setter)(const godot::String&)>
auto make_property(Obj* target)
{
	using T = godot::String;
	ConPropMethods<T> methods;
	methods.target = target;
	methods.getter = &ConPropMethods<T>::getterWithTarget<Obj, Getter>;
	methods.setter = &ConPropMethods<T>::setterWithTarget<Obj, Setter>;
	return methods;
}



struct SConCmdMethod
{
	typedef bool (*Function)(void*, const CmdExecArgs&);
	void* target;
	Function fn;
	
	// NO TARGET FUNCTIONS
	template <typename bool(*Func)(const CmdExecArgs&)>
	static bool make_notbound(void* target, const CmdExecArgs& args)
	{
		return Func(args);
	}
	template <typename void(*Func)(const CmdExecArgs&)>
	static bool make_notbound(void* target, const CmdExecArgs& args)
	{
		Func(args);
		return true;
	}
	template <typename bool(*Func)()>
	static bool make_notbound(void* target, const CmdExecArgs& args)
	{
		return Func();
	}
	template <typename void(*Func)()>
	static bool make_notbound(void* target, const CmdExecArgs& args)
	{
		Func();
		return true;
	}
	// BOUND METHODS
	template <typename T, typename void(T::*Func)()>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		(static_cast<T*>(target)->*Func)();
		return true;
	}
	template <typename T, typename bool(T::* Func)()>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		return (static_cast<T*>(target)->*Func)();
	}
	template <typename T, typename void(T::* Func)(const CmdExecArgs&)>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		(static_cast<T*>(target)->*Func)(args);
		return true;
	}
	template <typename T, typename bool(T::* Func)(const CmdExecArgs&)>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		return (static_cast<T*>(target)->*Func)(args);
	}
	// CONST QUALIFIED METHODS
	template <typename T, typename void(T::* Func)() const>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		(static_cast<T*>(target)->*Func)();
		return true;
	}
	template <typename T, typename bool(T::* Func)() const>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		return (static_cast<T*>(target)->*Func)();
	}
	template <typename T, typename void(T::* Func)(const CmdExecArgs&) const>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		(static_cast<T*>(target)->*Func)(args);
		return true;
	}
	template <typename T, typename bool(T::* Func)(const CmdExecArgs&) const>
	static bool make_bound(void* target, const CmdExecArgs& args)
	{
		return (static_cast<T*>(target)->*Func)(args);
	}
};

// TODO --> improve
template <typename Return, typename Arg, Return(*Func)(Arg) >
auto make_command()
{
	SConCmdMethod m;
	m.fn = &SConCmdMethod::make_notbound<Func>;
	return m;
}
template <typename Return, Return(*Func)() >
auto make_command()
{
	SConCmdMethod m;
	m.fn = &SConCmdMethod::make_notbound<Func>;
	return m;
}

template <bool(*Func)(const CmdExecArgs&)>
auto make_command()
{
	return make_command<bool, const CmdExecArgs&, Func>();
}
template <void(*Func)(const CmdExecArgs&)>
auto make_command()
{
	return make_command<void, const CmdExecArgs&, Func>();
}
template <bool(*Func)()>
auto make_command()
{
	return make_command<bool, Func>();

}
template <void(*Func)()>
auto make_command()
{
	return make_command<void, Func>();
}

template <typename T, typename bool(T::* Func)(const CmdExecArgs&)>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename void(T::* Func)(const CmdExecArgs&)>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename bool(T::* Func)(const CmdExecArgs&) const>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename void(T::* Func)(const CmdExecArgs&) const>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename bool(T::* Func)()>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename void(T::* Func)()>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename bool(T::* Func)() const>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
template <typename T, typename void(T::* Func)() const>
auto make_command(T* target)
{
	SConCmdMethod m;
	m.target = target;
	m.fn = &SConCmdMethod::make_bound<T, Func>;
	return m;
}
