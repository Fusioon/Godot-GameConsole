#pragma once

#include "Godot.hpp"

#include <unordered_map>
#include <queue>

#include "IConsole.h"

namespace godot {
	class ConsoleBindings;
}

namespace fusion {
	
	static bool tryParse(const godot::String& cmdLine, bool& value)
	{
		if (cmdLine == "true" || cmdLine == "True" || cmdLine == "TRUE" || cmdLine == "1") {
			value = true;
			return true;
		}
		if (cmdLine == "false" || cmdLine == "False" || cmdLine == "false" || cmdLine == "0") {
			value = true;
			return true;
		}
		return false;
	}
	static bool tryParse(const godot::String& cmdLine, int32_t& value)
	{
		if (cmdLine.is_valid_integer())
		{
			value = (int32_t)cmdLine.to_int();
			return true;
		}
		return false;
	}
	static bool tryParse(const godot::String& cmdLine, int64_t& value)
	{
		if (cmdLine.is_valid_integer())
		{
			value = cmdLine.to_int();
			return true;
		}
		return false;
	}
	static bool tryParse(const godot::String& cmdLine, float& value)
	{
		if (cmdLine.is_valid_float())
		{
			value = cmdLine.to_float();
			return true;
		}
		return false;
	}
	static bool tryParse(const godot::String& cmdLine, godot::String& value)
	{
		value = cmdLine;
		return true;
	}

	struct CVarBase : public ICVar
	{
		CVarBase(const godot::String& name, const godot::String& help, ECVarFlags flags) :
			m_name(name),
			m_help(help),
			m_flags(flags)
		{}
		virtual ~CVarBase() {  }
		virtual const godot::String& GetName() const override { return m_name; }
		virtual const godot::String& GetHelp() const override { return m_help; }
		virtual ECVarFlags GetFlags() const override { return m_flags; }
		virtual ECVarType GetType() const override { return ECVarType::Invalid; }
	protected:
		godot::String m_name;
		godot::String m_help;
		ECVarFlags m_flags;
	};

	template <typename T>
	struct SConVar : public CVarBase
	{
		SConVar(const godot::String& name, const godot::String& help, ECVarFlags flags, T* value, T default_value) :
			CVarBase(name, help, flags),
			m_pValue(value)
		{ }

		virtual bool GetValueBool() const = 0;
		virtual int32_t GetValueInt32() const = 0;
		virtual int64_t GetValueInt64() const = 0;
		virtual float GetValueFloat() const = 0;
		virtual godot::String GetValueString() const = 0;

		virtual bool Execute(const CmdExecArgs& args) override
		{
			T tmp;
			if (args.count > 1 && tryParse(args[1], tmp))
			{
				*m_pValue = tmp;
				return true;
			}
			return false;
		}
	protected:

		T* m_pValue;
	};

	struct SConVarBool : public SConVar<bool>
	{
		using SConVar::SConVar;

		virtual bool GetValueBool() const override { return *m_pValue; }
		virtual int32_t GetValueInt32() const override { return (int32_t)* m_pValue; }
		virtual int64_t GetValueInt64() const override { return (int64_t)* m_pValue; }
		virtual float GetValueFloat() const override { return (float)* m_pValue; }
		virtual godot::String GetValueString() const override { return *m_pValue ? "true" : "false"; }

		virtual ECVarType GetType() const override { return ECVarType::Bool; }

	};

	struct SConVarInt32 : public SConVar<int32_t>
	{
		using SConVar::SConVar;

		virtual bool GetValueBool() const override { return (bool)* m_pValue; }
		virtual int32_t GetValueInt32() const override { return *m_pValue; }
		virtual int64_t GetValueInt64() const override { return *m_pValue; }
		virtual float GetValueFloat() const override { return (float)* m_pValue; }
		virtual godot::String GetValueString() const override
		{
			return godot::String::num_int64(*m_pValue);
		}

		virtual ECVarType GetType() const override { return ECVarType::Int32; }

	};

	struct SConVarInt64 : public SConVar<int64_t>
	{
		using SConVar::SConVar;

		virtual bool GetValueBool() const override { return (bool)* m_pValue; }
		virtual int32_t GetValueInt32() const override { return (int32_t)* m_pValue; }
		virtual int64_t GetValueInt64() const override { return *m_pValue; }
		virtual float GetValueFloat() const override { return (float)* m_pValue; }
		virtual godot::String GetValueString() const override
		{
			return godot::String::num_int64(*m_pValue);
		}

		virtual ECVarType GetType() const override { return ECVarType::Int64; }

	};

	struct SConVarFloat : public SConVar<float>
	{
		using SConVar::SConVar;

		virtual bool GetValueBool() const override { return (bool)* m_pValue; }
		virtual int32_t GetValueInt32() const override { return (int32_t)* m_pValue; }
		virtual int64_t GetValueInt64() const override { return (int64_t)* m_pValue; }
		virtual float GetValueFloat() const override { return *m_pValue; }
		virtual godot::String GetValueString() const override
		{
			return godot::String::num_real(*m_pValue);
		}

		virtual ECVarType GetType() const override { return ECVarType::Float; }

	};

	struct SConVarString : public SConVar<godot::String>
	{
		using SConVar::SConVar;

		virtual bool GetValueBool() const override { return m_pValue->empty(); }
		virtual int32_t GetValueInt32() const override { return 0; }
		virtual int64_t GetValueInt64() const override { return 0; }
		virtual float GetValueFloat() const override { return 0; }
		virtual godot::String GetValueString() const override { return *m_pValue; }

		virtual ECVarType GetType() const override { return ECVarType::String; }

	};

	template <typename T>
	struct SConProp : public CVarBase
	{
		typedef T(*GetterMethod)(void*);
		typedef void(*SetterMethod)(void*, const T&);

		SConProp(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<T> methods, T default_value) :
			CVarBase(name, help, flags),
			m_target(methods.target),
			m_getter(methods.getter),
			m_setter(methods.setter)
		{ }

		virtual bool Execute(const CmdExecArgs& args) override
		{
			T tmp;
			if (args.count > 1 && tryParse(args[1], tmp))
			{
				if (m_setter)
				{
					m_setter(m_target, tmp);
				}
				return true;
			}
			return false;
		}

	protected:

		void* m_target;
		GetterMethod m_getter;
		SetterMethod m_setter;
	};

	struct SConPropBool : public SConProp<bool>
	{
		using SConProp::SConProp;

		virtual bool GetValueBool() const override { return (*m_getter)(m_target); }
		virtual int32_t GetValueInt32() const override { return (int32_t)(*m_getter)(m_target); }
		virtual int64_t GetValueInt64() const override { return (int64_t)(*m_getter)(m_target); }
		virtual float GetValueFloat() const override { return (float)(*m_getter)(m_target); }
		virtual godot::String GetValueString() const override { return (*m_getter)(m_target) ? "true" : "false"; }
		virtual ECVarType GetType() const override { return ECVarType::Bool; }
	};

	struct SConPropInt32 : public SConProp<int32_t>
	{
		using SConProp::SConProp;

		virtual bool GetValueBool() const override { return (bool)(*m_getter)(m_target); }
		virtual int32_t GetValueInt32() const override { return (int32_t)(*m_getter)(m_target); }
		virtual int64_t GetValueInt64() const override { return (int64_t)(*m_getter)(m_target); }
		virtual float GetValueFloat() const override { return (float)(*m_getter)(m_target); }
		virtual godot::String GetValueString() const override { return godot::String::num_int64((*m_getter)(m_target)); }
		virtual ECVarType GetType() const override { return ECVarType::Int32; }
	};

	struct SConPropInt64 : public SConProp<int64_t>
	{
		using SConProp::SConProp;

		virtual bool GetValueBool() const override { return (bool)(*m_getter)(m_target); }
		virtual int32_t GetValueInt32() const override { return (int32_t)(*m_getter)(m_target); }
		virtual int64_t GetValueInt64() const override { return (int64_t)(*m_getter)(m_target); }
		virtual float GetValueFloat() const override { return (float)(*m_getter)(m_target); }
		virtual godot::String GetValueString() const override { return godot::String::num_int64((*m_getter)(m_target)); }
		virtual ECVarType GetType() const override { return ECVarType::Int64; }
	};

	struct SConPropFloat : public SConProp<float>
	{
		using SConProp::SConProp;

		virtual bool GetValueBool() const override { return (bool)(*m_getter)(m_target); }
		virtual int32_t GetValueInt32() const override { return (int32_t)(*m_getter)(m_target); }
		virtual int64_t GetValueInt64() const override { return (int64_t)(*m_getter)(m_target); }
		virtual float GetValueFloat() const override { return (float)(*m_getter)(m_target); }
		virtual godot::String GetValueString() const override { return godot::String::num_real((*m_getter)(m_target)); }
		virtual ECVarType GetType() const override { return ECVarType::Float; }
	};

	struct SConPropString : public SConProp<godot::String>
	{
		using SConProp::SConProp;

		virtual bool GetValueBool() const override { return (*m_getter)(m_target).empty(); }
		virtual int32_t GetValueInt32() const override { return 0; }
		virtual int64_t GetValueInt64() const override { return 0; }
		virtual float GetValueFloat() const override { return 0; }
		virtual godot::String GetValueString() const override { return (*m_getter)(m_target); }
		virtual ECVarType GetType() const override { return ECVarType::String; }
	};

	struct SConCmd : public CVarBase
	{
		typedef bool (*Function)(void*, const CmdExecArgs&);

		SConCmd(const godot::String& name, const godot::String& help, ECVarFlags flags, SConCmdMethod cmdfn) :
			CVarBase(name, help, flags),
			m_pTarget(cmdfn.target),
			m_function(cmdfn.fn)
		{}

		virtual ECVarType GetType() const override { return ECVarType::Command; }

		virtual bool GetValueBool() const override { return false; }
		virtual int32_t GetValueInt32() const override { return 0; }
		virtual int64_t GetValueInt64() const override { return 0; }
		virtual float GetValueFloat() const override { return 0; }
		virtual godot::String GetValueString() const override { return godot::String("Command"); }
		virtual bool Execute(const CmdExecArgs& args) override
		{
			return m_function(m_pTarget, args);
		}
	private:
		void* m_pTarget;
		Function m_function;
	};

	class Console
	{
		struct equality
		{
			typedef godot::String argument_type;
			//typedef std::size_t result_type;
			bool operator()(const argument_type& lhs, const argument_type& rhs) const noexcept
			{
				return lhs.nocasecmp_to(rhs) == 0;
			}
		};

		struct hash
		{
			typedef godot::String argument_type;
			typedef std::size_t result_type;
			result_type operator()(argument_type const& s) const noexcept
			{
				return s.hash();
			}
		};

	public:
		struct CommandHistory
		{
			static constexpr int k_max_history_size = 64;

			CommandHistory();
			~CommandHistory();

			void add(const godot::String& str);
			const godot::String& get();
			const godot::String& up();
			const godot::String& down();

		private:
			godot::String* m_history;
			int history_next_idx;
			int history_selected_idx;
		};

		struct Autocomplete
		{
			
		private:
			ICVar* cvars;
		};

		friend class godot::ConsoleBindings;

		Console();
		~Console();

		ICVar* RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, bool* value, bool default_value);
		ICVar* RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, int32_t* value, int32_t default_value);
		ICVar* RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, int64_t* value, int64_t default_value);
		ICVar* RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, float* value, float default_value);
		ICVar* RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, godot::String* value, const godot::String& default_value);

		ICVar* RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<bool> methods, bool default_value);
		ICVar* RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<int32_t> methods, int32_t default_value);
		ICVar* RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<int64_t> methods, int64_t default_value);
		ICVar* RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<float> methods, float default_value);
		ICVar* RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<godot::String> methods, const godot::String& default_value);

		ICVar* RegisterCommand(const godot::String& name, const godot::String& help, ECVarFlags flags, SConCmdMethod fn);

		bool UnRegisterCVar(const godot::String& name);
		ICVar* GetCVar(const godot::String& name) const;
		
		void EnqueueCommand(const godot::String& cmd);
		void EnqueueCommandNoHistory(const godot::String& cmd);
		void ExecuteString(const godot::String& cmd, bool silent, bool deferred);

		
		size_t CVarCount() const { return m_cvars.size(); }

		CommandHistory& GetHistory() { return m_history; }


		void Open();
		void Close();
		void PrintLine(const godot::String& text, ELogType type);
		void PrintInfo(const godot::String& text);
		void PrintSuccess(const godot::String& text);
		void PrintWarning(const godot::String& text);
		void PrintError(const godot::String& text);
		void PrintException(const godot::String& text);

		void SetUI(IConsoleUI* pUi);

	
		void RegisterAutoComplete(const godot::String& sVarOrCommand, IConsoleArgumentAutoComplete* pArgAutoComplete);
		void UnRegisterAutoComplete(const godot::String& sVarOrCommand);
		void ResetAutoCompletion();

		size_t GetSuggestions(const godot::String& prefix, ICVar** target_arr, size_t target_size);

	private:
		ICVar* RegisterCVar(ICVar* pCVar);

		void Update(float dt);
		bool UpdateWait(float dt);
		bool ExecuteString_Impl(const godot::String& cmd);
		ICVar* CanRegisterName(const godot::String& name);

		std::unordered_map<godot::String, ICVar*, hash, equality> m_cvars;
		std::unordered_map<godot::String, IConsoleArgumentAutoComplete*, hash, equality> m_argAutoComplete;

		std::queue<godot::String> m_command_queue;
		

		int wait_frames;
		float wait_seconds;
		bool wait_level_load;

		float get_wait_seconds();
		void set_wait_seconds(float value);

		int get_wait_frames();
		void set_wait_frames(int value);

		bool get_wait_level_load();
		void set_wait_level_load(bool value);

		void help_command(const CmdExecArgs& args);
		void print_command(const CmdExecArgs& args);
		void commands_command(const CmdExecArgs& args);
		void variables_command(const CmdExecArgs& args);
		void clear_command(const CmdExecArgs& args);
		void execute_command(const CmdExecArgs& args);
		size_t m_max_lines;

		CommandHistory m_history;
		IConsoleUI* m_pUi;
	};

	extern class Console* pConsole;
};

