#include "Godot.hpp"
#include "FusionConsole.hpp"
#include "gdnative_api_struct.gen.h"
#include <algorithm>
#include "File.hpp"
#include "Directory.hpp"
using namespace fusion;

//godot::String Console::CVarNameFinder::s_help = "CVarNameFinder";

struct SExecAutocomplete : IConsoleArgumentAutoComplete {

	SExecAutocomplete() {
		m_dir = godot::Directory::_new();
		auto err = m_dir->open("res://config/");
		if (err == godot::Error::OK) {

		}
	}

	virtual bool Autocomplete(const godot::String& input, godot::String& result) const {
		result = m_dir->get_next();
		if (result.begins_with(input)) {

		}
		return result.empty();
	}

private:
	godot::Directory* m_dir;
};

template <typename T>
constexpr T* memnew()
{
	return static_cast<T*>(godot::api->godot_alloc(sizeof(T)))();
}

template <typename T>
constexpr T* memnew(const T& obj)
{
	return static_cast<T*>(godot::api->godot_alloc(sizeof(T)))(obj);
}

template <typename T>
constexpr T* memnew(T&& obj)
{
	return static_cast<T*>(godot::api->godot_alloc(sizeof(T)))(obj);
}

template <typename T>
constexpr T* memnew_arr(size_t size)
{
	return new (static_cast<T*>(godot::api->godot_alloc(sizeof(T) * size))) T[size];
}

template <typename T>
constexpr void memdel(T* target)
{
	target->~T();
	godot::api->godot_free(target);
}

template <typename T>
constexpr void memdel_arr(T* target, size_t size)
{
	for (T* it = target; it != target + size; ++it) it->~T();
	godot::api->godot_free(target);
	new T[size];
}

template <typename _Ty>
constexpr _Ty clamp(_Ty current, _Ty min, _Ty max) {
	return current < min ? min : current > max ? max : current;
}

bool is_whitespace(wchar_t c)
{
	return c == ' ' || c == '\t';
}

void skip_whitespace(const godot::String& cmdLine, int32_t& pos)
{
	while (pos < cmdLine.length() && is_whitespace(cmdLine[pos]))
	{
		++pos;
	}
}
void skip_quotes(const godot::String& cmdLine, int32_t& pos)
{
	while (pos < cmdLine.length())
	{
		++pos;
		if (cmdLine[pos] == '\"' && cmdLine[pos - 1] != '\\') {
			++pos;
			break;
		}
	}
}

godot::String parse(const godot::String& cmdLine, int32_t& pos)
{
	int startpos = pos;
	while (pos < cmdLine.length())
	{
		if (is_whitespace(cmdLine[pos]))
		{
			return cmdLine.substr(startpos, pos - startpos);
		}
		++pos;
	}
	return cmdLine.substr(startpos, cmdLine.length());
}

godot::String parse_quotes(const godot::String& cmdLine, int32_t& pos)
{
	int startpos = ++pos;
	while (pos < cmdLine.length())
	{
		if (cmdLine[pos] == '"' && cmdLine[pos - 1] != '\\')
		{
			return cmdLine.substr(startpos, pos++ - startpos);
		}
		++pos;
	}
	return cmdLine.substr(startpos, cmdLine.length());
}

bool tokenize(const godot::String& cmdLine, godot::String& result, int32_t& pos)
{
	while (pos < cmdLine.length())
	{
		skip_whitespace(cmdLine, pos);
		if (pos == cmdLine.length()) break;


		if (cmdLine[pos] == '\"')
		{
			result = (parse_quotes(cmdLine, pos));
			return true;
		}
		else
		{
			result = (parse(cmdLine, pos));
			return true;
		}
	}


	return false;
}

bool break_multiple_commands(const godot::String input, godot::String& result, int32_t& pos)
{
	constexpr char BREAK_CHAR = ';';
	int32_t start_pos = pos;
	bool whitespace_only = true; // when we check if string has only whitespaces we can prevent unnecessary allocations

	while (pos < input.length())
	{
		if (input[pos] == '\"')
		{
			auto start = pos;
			skip_quotes(input, pos);
			continue;
		}
		if (input[pos] == BREAK_CHAR)
		{
			auto length = pos - start_pos;
			if (length > 0 && !whitespace_only)
			{
				whitespace_only = true;
				result = input.substr(start_pos, length);
				++pos;
				return true;
			}
			start_pos = ++pos;
			continue;
		}

		if (whitespace_only) {
			if (is_whitespace(input[pos])) {
				start_pos = ++pos;
				continue;
			}
			else {
				whitespace_only = false;
			}
		}
		++pos;
	}
	auto length = pos - start_pos;
	if (length > 0 && !whitespace_only)
	{
		result = input.substr(start_pos, length);
		return true;
	}
	return false;
}

Console::CommandHistory::CommandHistory() :
	//m_history(memnew_arr<godot::String>(k_max_history_size+1))
	history_next_idx(0),
	history_selected_idx(0),
	m_history(new godot::String[k_max_history_size + 1])
{
	m_history[k_max_history_size] = godot::String();
}

Console::CommandHistory::~CommandHistory()
{
	delete[] m_history;
}

void Console::CommandHistory::add(const godot::String& str)
{
	m_history[history_next_idx++ % k_max_history_size] = str;
	history_selected_idx = 0;
	if (history_next_idx > k_max_history_size)
	{
		history_next_idx = history_next_idx % k_max_history_size + k_max_history_size;
	}
}

const godot::String& Console::CommandHistory::get()
{
	history_selected_idx = clamp(history_selected_idx, 0, std::min(history_next_idx, k_max_history_size));
	if (history_selected_idx <= 0) {
		return m_history[k_max_history_size];
	}

	return m_history[(history_next_idx - history_selected_idx) % k_max_history_size];
}

const godot::String& Console::CommandHistory::up()
{
	++history_selected_idx;
	return get();
}

const godot::String& Console::CommandHistory::down()
{
	--history_selected_idx;
	return get();
}


Console::Console()
{
	RegisterVariable("wait.seconds", "waits for level load before executing next command in queue.", ECVarFlags::Null, &wait_seconds, 0);
	RegisterVariable("wait.frames", "waits for level load before executing next command in queue.", ECVarFlags::Null, &wait_frames, 0);
	RegisterVariable("wait.sceneload", "waits for level load before executing next command in queue.", ECVarFlags::Null, &wait_level_load, false);
	RegisterCommand("help", "", ECVarFlags::Null, make_command<Console, &Console::help_command>(this));
	RegisterCommand("close", "", ECVarFlags::Null, make_command < Console, &Console::Close>(this));
	RegisterCommand("print", "", ECVarFlags::Null, make_command < Console, &Console::print_command>(this));
	RegisterCommand("cmds", "", ECVarFlags::Null, make_command < Console, &Console::commands_command>(this));
	RegisterCommand("vars", "", ECVarFlags::Null, make_command < Console, &Console::variables_command>(this));
	RegisterCommand("cls", "", ECVarFlags::Null, make_command < Console, &Console::clear_command>(this));
	RegisterCommand("exec", "", ECVarFlags::Null, make_command < Console, &Console::execute_command>(this));
}

Console::~Console()
{
	for (auto it : m_cvars)
	{
		delete it.second;
	}
	m_cvars.clear();
}

bool Console::UnRegisterCVar(const godot::String& name)
{
	auto result = m_cvars.find(name);
	if (result != m_cvars.end())
	{
		delete result->second;
		m_cvars.erase(result);
		return true;
	}
	return false;
}

ICVar* Console::GetCVar(const godot::String& name) const
{
	auto result = m_cvars.find(name);
	if (result != m_cvars.end()) {
		return result->second;
	}
	return nullptr;
}

void Console::EnqueueCommand(const godot::String& cmdLine)
{
	m_history.add(cmdLine);
	EnqueueCommandNoHistory(cmdLine);
}

void Console::EnqueueCommandNoHistory(const godot::String& cmdLine)
{
	godot::String result;
	int32_t pos = 0;
	while (break_multiple_commands(cmdLine, result, pos))
	{
		m_command_queue.emplace(result);
	}
}

void Console::ExecuteString(const godot::String& cmd, bool silent, bool deferred)
{
	if (deferred) {
		if (silent) {
			EnqueueCommandNoHistory(cmd);
		}
		else {
			EnqueueCommand(cmd);
		}
	}
	else {
		ExecuteString_Impl(cmd);
	}
}

bool Console::UpdateWait(float dt)
{
	if (wait_frames > 0)
	{
		--wait_frames;
		return true;
	}
	if (wait_seconds > 0)
	{
		wait_seconds -= dt;
		return true;
	}
	if (wait_level_load)
	{
		if (false /*!LEVEL_LOADED*/)
		{
			return true;
		}
		wait_level_load = false;
	}


	return false;
}

void Console::Update(float dt)
{
	while (!UpdateWait(dt) && !m_command_queue.empty()) {
		auto& command = m_command_queue.front();
		godot::Godot::print(command);
		if (!ExecuteString_Impl(command)) {
			godot::Godot::print("Error while executing \"" + command + "\"");
		}
		m_command_queue.pop();
	}
}

bool Console::ExecuteString_Impl(const godot::String& cmdLine)
{
	int32_t pos = 0;
	godot::String arg;
	if (tokenize(cmdLine, arg, pos)) {
		auto result = m_cvars.find(arg);
		if (result != m_cvars.end()) {
			std::vector<godot::String> tmp;
			tmp.push_back(arg);
			while (tokenize(cmdLine, arg, pos)) {
				tmp.push_back(arg);
			}
			ICVar* pCVar = result->second;
			if (pCVar->GetType() != ECVarType::Command && tmp.size() == 1) {
				godot::Godot::print("{0} {1}", pCVar->GetName(), pCVar->GetValueString());
				return true;
			}
			if (tmp.size() == 2 && tmp[1] == "?") {
				if (pCVar->GetType() != ECVarType::Command) {
					godot::Godot::print("{0} - {1}", pCVar->GetName(), pCVar->GetHelp());
				}
				else {
					godot::Godot::print("{0} {1} - {2}", pCVar->GetName(), pCVar->GetValueString(), pCVar->GetHelp());
				}

				return true;
			}

			CmdExecArgs args(cmdLine, &tmp[0], tmp.size());
			return pCVar->Execute(args);
		}
		else {
			godot::Godot::print("Failed to find \"" + arg + "\" in registered cvars");
			PrintError("Failed to find \"" + arg + "\" in registered cvars");
		}
	}
	else {
		godot::Godot::print("Failed to parse line: \"" + cmdLine + "\"");
		PrintError("Failed to parse line: \"" + cmdLine + "\"");

	}
	return true;
}


ICVar* Console::CanRegisterName(const godot::String& name)
{
	auto result = m_cvars.find(name);
	if (result != m_cvars.end()) {
		PrintWarning(godot::String("[DUPLICATE] variable [{0}] is already registered.").format(godot::Array::make(name)));
		return result->second;
	}
	return nullptr;
}

ICVar* Console::RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, bool* value, bool default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConVarBool(name, help, flags, value, default_value);
	m_cvars[name] = cvar;
	return cvar;
}
ICVar* Console::RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, int32_t* value, int32_t default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConVarInt32(name, help, flags, value, default_value);
	m_cvars[name] = cvar;
	return cvar;
}
ICVar* Console::RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, int64_t* value, int64_t default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConVarInt64(name, help, flags, value, default_value);
	m_cvars[name] = cvar;
	return cvar;
}
ICVar* Console::RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, float* value, float default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConVarFloat(name, help, flags, value, default_value);
	m_cvars[name] = cvar;
	return cvar;
}
ICVar* Console::RegisterVariable(const godot::String& name, const godot::String& help, ECVarFlags flags, godot::String* value, const godot::String& default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConVarString(name, help, flags, value, default_value);
	m_cvars[name] = cvar;
	return cvar;
}

ICVar* Console::RegisterCommand(const godot::String& name, const godot::String& help, ECVarFlags flags, SConCmdMethod fn)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConCmd(name, help, flags, fn);
	return m_cvars[name] = cvar;;
}


ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<bool> methods, bool default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConPropBool(name, help, flags, methods, default_value);
	return m_cvars[name] = cvar;;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<int32_t> methods, int32_t default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConPropInt32(name, help, flags, methods, default_value);
	return m_cvars[name] = cvar;;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<int64_t> methods, int64_t default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConPropInt64(name, help, flags, methods, default_value);
	return m_cvars[name] = cvar;;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<float> methods, float default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConPropFloat(name, help, flags, methods, default_value);
	return m_cvars[name] = cvar;;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<godot::String> methods, const godot::String& default_value)
{
	auto cvar = CanRegisterName(name);
	if (cvar) {
		return cvar;
	}

	cvar = new SConPropString(name, help, flags, methods, default_value);
	return m_cvars[name] = cvar;;
}

ICVar* Console::RegisterCVar(ICVar* pCVar)
{
	auto cvar = CanRegisterName(pCVar->GetName());
	if (cvar) {
		return cvar;
	}
	return m_cvars[pCVar->GetName()] = pCVar;
}


void Console::Open() {
	if (m_pUi) m_pUi->Open();
}
void Console::Close() {
	if (m_pUi) m_pUi->Close();

}
void Console::PrintLine(const godot::String& text, ELogType type) {
	if (m_pUi) m_pUi->PrintLine(text, type);
}

void Console::PrintInfo(const godot::String& text) {
	PrintLine(text, ELogType::Null);
}

void Console::PrintSuccess(const godot::String& text) {
	PrintLine(text, ELogType::Success);
}

void Console::PrintWarning(const godot::String& text) {
	PrintLine(text, ELogType::Warning);
}

void Console::PrintError(const godot::String& text) {
	PrintLine(text, ELogType::Error);
}

void Console::PrintException(const godot::String& text) {
	PrintLine(text, ELogType::Exception);
}


void Console::SetUI(IConsoleUI* pUi) {
	m_pUi = pUi;
}
//
//Console::Autocomplete Console::GetAutoComplete(const godot::String& value) {
//	return Autocomplete(m_cvars.begin(), m_cvars.end(), value);
//}

size_t Console::GetSuggestions(const godot::String& prefix, ICVar** target_arr, size_t target_size) {
	size_t count = 0;
	for (auto c : m_cvars) {
		if (count >= target_size) {
			break;
		}
		if (c.first.begins_with(prefix)) {
			target_arr[count] = c.second;
			++count;
		}
	}
	return count;
}

///-----
void Console::set_wait_seconds(float value) {
	wait_seconds = std::max(0.0f, value);
}
float Console::get_wait_seconds() {
	return wait_seconds;
}

int Console::get_wait_frames() {
	return wait_frames;
}
void Console::set_wait_frames(int value) {
	wait_frames = std::max(0, value);
}

bool Console::get_wait_level_load() {
	return wait_level_load;
}
void Console::set_wait_level_load(bool value) {
	wait_level_load = value;
}


void Console::help_command(const CmdExecArgs& args) {
	godot::String filter = godot::String();
	if (args.count > 1) {
		filter = args[1];
	}

	auto print_cvar = [this](ICVar* pCvar) -> void {
		auto name = pCvar->GetName();
		auto help = pCvar->GetHelp();
		if (help.empty()) {
			PrintInfo(name);
		}
		else {
			PrintInfo(godot::String("{0} - {1}").format(godot::Array::make(pCvar->GetName(), pCvar->GetHelp())));
		}
	};

	if (filter.empty()) {
		for (auto cvar : m_cvars) {
			print_cvar(cvar.second);
		}
	}
	else {
		for (auto cvar : m_cvars) {
			auto name = cvar.first;
			if (name.find(filter) < 0) {
				continue;
			}
			print_cvar(cvar.second);
		}
	}
}

void Console::print_command(const CmdExecArgs& args) {
	auto line = args.line_remove_command();
	auto quote = godot::String("\"");
	if (line.length() >= 2 && line.begins_with(quote) && line.ends_with(quote)) {
		PrintInfo(line.substr(1, line.length() - 2));
	}
	else {
		PrintInfo(line);
	}
}
void Console::commands_command(const CmdExecArgs& args) {

}
void Console::variables_command(const CmdExecArgs& args) {

}

void Console::clear_command(const CmdExecArgs& args) {
	if (m_pUi) m_pUi->Clear();
}

void Console::execute_command(const CmdExecArgs& args) {
	// TODO
	if (args.count >= 2) {
		auto file_path = args.line_remove_command();
		godot::File* f = godot::File::_new();

		if (f->file_exists(file_path)) {
			auto err = f->open(file_path, godot::File::READ);
			if (err == godot::Error::OK) {
				while (!f->eof_reached()) {
					auto line = f->get_line();
					EnqueueCommandNoHistory(line);
				}
				f->close();
			}
			else {
				if (err == godot::Error::ERR_FILE_NOT_FOUND) {
					PrintError("Failed to execute file \"" + file_path + "\". FILE_NOT_FOUND");
				}
			}
		}
	}
}