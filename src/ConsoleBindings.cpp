#include "ConsoleBindings.hpp"
#include "FusionConsole.hpp"
#include "ConsoleUI.hpp"

#include "PackedScene.hpp"
#include "ResourceLoader.hpp"
#include "SceneTree.hpp"
#include "Viewport.hpp"

using namespace godot;


class GDSCVarBase : public fusion::CVarBase
{

public:
	GDSCVarBase(const String& name, const String& help, ECVarFlags flags, Reference* ref, const String& member_name) :
		CVarBase(name, help, flags),
		m_ref(ref),
		m_member(member_name)
	{ }
protected:
	Reference* m_ref;
	String m_member;
};

template <typename T>
class GDSConVarBase : public GDSCVarBase
{
public:
	using GDSCVarBase::GDSCVarBase;
	virtual bool GetValueBool() const { return (bool)m_ref->get(m_member); }
	virtual int32_t GetValueInt32() const { return (int32_t)m_ref->get(m_member); }
	virtual int64_t GetValueInt64() const { return (int64_t)m_ref->get(m_member); }
	virtual float GetValueFloat() const { return (float)m_ref->get(m_member); }
	virtual godot::String GetValueString() const { return (godot::String)m_ref->get(m_member); }

	virtual bool Execute(const ::CmdExecArgs& args)
	{
		T tmp;
		if (args.count > 1 && fusion::tryParse(args[1], tmp))
		{
			m_ref->set(m_name, tmp);
			return true;
		}
		return false;
	}
};

void godot::CmdArgs::_register_methods()
{
	register_method("_init", &CmdArgs::_init);
	register_method("at", &CmdArgs::arg);
	register_property<CmdArgs, int64_t>("count", &CmdArgs::set_count, &CmdArgs::count, 0);
	register_property<CmdArgs, String>("line", &CmdArgs::set_line, &CmdArgs::line, "");
	register_property<CmdArgs, String>("line_remove_command", &CmdArgs::set_line, &CmdArgs::line_remove_command, "");
}

class GDSConCmd : public GDSCVarBase
{
public:
	using GDSCVarBase::GDSCVarBase;

	virtual bool GetValueBool() const { return false; }
	virtual int32_t GetValueInt32() const { return 0; }
	virtual int64_t GetValueInt64() const { return 0; }
	virtual float GetValueFloat() const { return 0; }
	virtual godot::String GetValueString() const { return "GDScript command"; }
	virtual ECVarType GetType() const { return ECVarType::Command; }
	
	virtual bool Execute(const ::CmdExecArgs& p_args)
	{
		godot::CmdArgs* args = godot::CmdArgs::_new();
		args->m_args = &p_args;
		Variant result = m_ref->call(m_member, args);
		return result.get_type() == Variant::NIL ? true : (bool)result;
	}
};


void ConsoleBindings::_register_methods()
{
	godot::register_method("_init", &ConsoleBindings::_init);
	godot::register_method("_ready", &ConsoleBindings::_ready);
	godot::register_method("_process", &ConsoleBindings::_process);
	godot::register_method("enqueue_command", &ConsoleBindings::EnqueueCommand);
	godot::register_method("register_variable", &ConsoleBindings::RegisterVariable);
	godot::register_method("register_property", &ConsoleBindings::RegisterProperty);
	godot::register_method("register_command", &ConsoleBindings::RegisterCommand);
	godot::register_method("unregister_cvar", &ConsoleBindings::UnregisterCVar);

	godot::register_method("history_add", &ConsoleBindings::HistoryAdd);
	godot::register_method("history_up", &ConsoleBindings::HistoryUp);
	godot::register_method("history_down", &ConsoleBindings::HistoryDown);
	godot::register_method("history_get", &ConsoleBindings::HistoryGet);

	godot::register_method("open", &ConsoleBindings::Open);
	godot::register_method("close", &ConsoleBindings::Close);
	godot::register_method("quit", &ConsoleBindings::Quit);
}

void ConsoleBindings::_init() {
	fusion::pConsole->RegisterCommand("quit", "", ECVarFlags::Null, make_command<ConsoleBindings, &ConsoleBindings::Quit>(this));
}

void ConsoleBindings::_ready() {

	Ref<PackedScene> scene = ResourceLoader::get_singleton()->load("res://Console/ConsoleUI.tscn");
	if (!scene.is_null()) {
		(void)get_tree()->get_root()->call_deferred("add_child", scene->instance(), true);
	}
	
}

void ConsoleBindings::_process(float dt) {
	fusion::pConsole->Update(dt);
}

void ConsoleBindings::EnqueueCommand(godot::String cmd_line) {
	fusion::pConsole->EnqueueCommand(cmd_line);
}


template <>
struct _ArgCast<ECVarFlags> {
	static ECVarFlags _arg_cast(Variant a) {
		return EnumUtils<ECVarFlags>::to_enum(a);
	}
};

bool ConsoleBindings::RegisterVariable(String cvar_name, String cvar_help, ECVarFlags flags, Reference* ref, String property_name) {
	ICVar* tmp;
	Variant::Type type = ref->get(property_name).get_type();

	switch (type)
	{
	case godot::Variant::BOOL:
		tmp = new GDSConVarBase<bool>(cvar_name, cvar_help, flags, ref, property_name);
		break;
	case godot::Variant::INT:
		tmp = new GDSConVarBase<int64_t>(cvar_name, cvar_help, flags, ref, property_name);
		break;
	case godot::Variant::REAL:
		tmp = new GDSConVarBase<float>(cvar_name, cvar_help, flags, ref, property_name);
		break;
	case godot::Variant::STRING:
		tmp = new GDSConVarBase<String>(cvar_name, cvar_help, flags, ref, property_name);
		break;

	default:
		
		tmp = nullptr;
		break;
	}

	if (tmp != nullptr && tmp != fusion::pConsole->RegisterCVar(tmp)) {
		Godot::print("failed to register...");
		delete tmp;
		return false;
	}
	return tmp != nullptr;
}

bool ConsoleBindings::RegisterProperty(String cvar_name, String cvar_help, ECVarFlags flags_int, Reference* ref, String property_name) {
	return false;
}

bool ConsoleBindings::RegisterCommand(String command_name, String cvar_help, ECVarFlags flags_int, Reference* ref, String method_name) {
	ICVar* tmp = new GDSConCmd(command_name, cvar_help, flags_int, ref, method_name);
	if (tmp != fusion::pConsole->RegisterCVar(tmp)) {
		Godot::print("failed to register...");
		delete tmp;
		return false;
	}
	return true;
}

bool ConsoleBindings::UnregisterCVar(String cvar_name) {
	return fusion::pConsole->UnRegisterCVar(cvar_name);
}

String ConsoleBindings::HistoryUp() const {
	return fusion::pConsole->GetHistory().up();
}

String ConsoleBindings::HistoryDown() const {
	return fusion::pConsole->GetHistory().down();
}

String ConsoleBindings::HistoryGet() const {
	return fusion::pConsole->GetHistory().get();
}

void ConsoleBindings::HistoryAdd(String cmdLine) {
	fusion::pConsole->GetHistory().add(cmdLine);
}

void ConsoleBindings::Open() {
	fusion::pConsole->Open();
}

void ConsoleBindings::Close() {
	fusion::pConsole->Close();
}

void ConsoleBindings::Quit() {
	get_tree()->quit();
}