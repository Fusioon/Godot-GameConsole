#pragma once

#include "Godot.hpp"
#include "Node.hpp"

#include "IConsole.h"


namespace godot {

	class CmdArgs : public Reference
	{
		GODOT_CLASS(CmdArgs, Reference);
	public:
		const CmdExecArgs* m_args;

		static void _register_methods();

		CmdArgs() : m_args(nullptr) {}
		
		void _init() {}
		
		int64_t count() const { return m_args->count; }
		void set_count(int64_t value) {}
		String line() const { return m_args->line; }
		void set_line(String value) { }
		String arg(int32_t i) const { return m_args->arg(i); }
		String line_remove_command() const { return m_args->line_remove_command(); }
	};

	class ConsoleBindings : public Node
	{
		GODOT_CLASS(ConsoleBindings, Node);

	public:
		static void _register_methods();
		void _init();
		void _ready();
		void _process(float dt);
		void EnqueueCommand(String cmd_line);
		bool RegisterVariable(String cvar_name, String cvar_help, ECVarFlags flags, Reference* ref, String property_name);
		bool RegisterProperty(String cvar_name, String cvar_help, ECVarFlags flags, Reference* ref, String property_name);
		bool RegisterCommand(String command_name, String cvar_help, ECVarFlags flags, Reference* ref, String method_name);
		bool UnregisterCVar(String cvar_name);

		String HistoryUp() const;
		String HistoryDown() const;
		String HistoryGet() const;
		void HistoryAdd(String cmdLine);

		void Open();
		void Close();
		void Quit();
	};
}



