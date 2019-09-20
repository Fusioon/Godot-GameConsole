#include "Godot.hpp"
#include "FusionConsole.hpp"
#include "gdnative_api_struct.gen.h"
using namespace fusion;

const size_t Console::CommandHistory::k_max_history_size = 64;

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
	return static_cast<T*>(godot::api->godot_alloc(sizeof(T) * size));
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
		if (cmdLine[pos] == '\"' && cmdLine[pos-1] != '\\') {
			++pos;
			break;
		}
		++pos;
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
	bool in_quotes = false;
	int32_t start_pos = pos;
	bool whitespace_only = true; // when we check if string has only whitespaces we can prevent unnecessary allocations

	while (pos < input.length())
	{
		if (input[pos] == '\"')
		{
			skip_quotes(input, pos);
			continue;
		}
		if (input[pos] == BREAK_CHAR && !in_quotes)
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
	m_history(new godot::String[k_max_history_size + 1])
{
	m_history[k_max_history_size] = godot::String();
}

Console::CommandHistory::~CommandHistory()
{
	delete[] m_history;
	//memdel_arr(m_history, k_max_history_size+1);
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
	if (history_selected_idx <= 0) {
		return m_history[k_max_history_size];
	}
	size_t min = std::min(history_next_idx, k_max_history_size);
	history_selected_idx = history_selected_idx > min ? min : history_selected_idx;
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
	RegisterProperty("wait.seconds", "waits seconds before executing next command in queue.", ECVarFlags::Null,
		make_property<Console, &Console::get_wait_seconds, &Console::set_wait_seconds>(this), 0);
}

Console::~Console()
{
	for (auto it : m_cvars)
	{
		delete it;
	}
	m_cvars.clear();
}

bool Console::UnregisterCVar(const godot::String& name)
{
	auto finder = CVarNameFinder(name);
	auto result = m_cvars.find(&finder);
	if (result != m_cvars.end())
	{
		delete* result;
		m_cvars.erase(result);
		return true;
	}
	return false;
}

ICVar* Console::GetCVar(const godot::String& name) const
{
	auto finder = CVarNameFinder(name);
	auto result = m_cvars.find(&finder);
	if (result != m_cvars.end()) {
		return *result;
	}
	return nullptr;
}

void Console::EnqueueCommand(const godot::String& cmdLine)
{
	// TODO -> add line to history
	EnqueueCommandNoHistory(cmdLine);
}

void Console::EnqueueCommandNoHistory(const godot::String& cmdLine)
{
	godot::String result;
	int32_t pos = 0;
	while(break_multiple_commands(cmdLine, result, pos))
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
		auto finder = CVarNameFinder(arg);
		auto result = m_cvars.find(&finder);
		if (result != m_cvars.end()) {
			std::vector<godot::String> tmp;
			while (tokenize(cmdLine, arg, pos)) {
				tmp.push_back(arg);
			}
			if (tmp.size() == 0) {
				CmdExecArgs args(cmdLine, nullptr, 0);
				return (*result)->Execute(args);
			}

			CmdExecArgs args(cmdLine, &tmp[0], tmp.size());
			return (*result)->Execute(args);
		}
		else {
			godot::Godot::print("Failed to find \"" + arg + "\" in registered cvars");
		}
	}
	else {
		godot::Godot::print("Failed to parse line: \"" + cmdLine + "\"");
	}
	return false;
}


bool Console::CanRegisterName(const godot::String& name)
{
	auto finder = CVarNameFinder(name);
	auto result = m_cvars.find(&finder);
//#if 1 || _DEBUG
	if (result != m_cvars.end()) {
		godot::Godot::print_error("CVar with name \"" + name + "\" is already defined", __func__, __FILE__, __LINE__);
	}
//#endif
	return (result == m_cvars.end()); 
}

ICVar* Console::RegisterCVar(const godot::String& name, const godot::String& help, ECVarFlags flags, bool* value, bool default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConVarBool(name, help, flags, value, default_value)).first);
	}
	return nullptr;
}
ICVar* Console::RegisterCVar(const godot::String& name, const godot::String& help, ECVarFlags flags, int32_t* value, int32_t default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConVarInt32(name, help, flags, value, default_value)).first);
	}
	return nullptr;
}
ICVar* Console::RegisterCVar(const godot::String& name, const godot::String& help, ECVarFlags flags, int64_t* value, int64_t default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConVarInt64(name, help, flags, value, default_value)).first);
	}
	return nullptr;
}
ICVar* Console::RegisterCVar(const godot::String& name, const godot::String& help, ECVarFlags flags, float* value, float default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConVarFloat(name, help, flags, value, default_value)).first);
	}
	return nullptr;
}
ICVar* Console::RegisterCVar(const godot::String& name, const godot::String& help, ECVarFlags flags, godot::String* value, const godot::String& default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConVarString(name, help, flags, value, default_value)).first);
	}
	return nullptr;
}

ICVar* Console::RegisterCommand(const godot::String& name, const godot::String& help, ECVarFlags flags, SConCmdMethod fn)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConCmd(name, help, flags, fn)).first);

	}
	return nullptr;
}


ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<bool> methods, bool default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConPropBool(name, help, flags, methods, default_value)).first);
	}
	return nullptr;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<int32_t> methods, int32_t default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConPropInt32(name, help, flags, methods, default_value)).first);
	}
	return nullptr;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<int64_t> methods, int64_t default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConPropInt64(name, help, flags, methods, default_value)).first);
	}
	return nullptr;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<float> methods, float default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConPropFloat(name, help, flags, methods, default_value)).first);
	}
	return nullptr;
}

ICVar* Console::RegisterProperty(const godot::String& name, const godot::String& help, ECVarFlags flags, ConPropMethods<godot::String> methods, const godot::String& default_value)
{
	if (CanRegisterName(name)) {
		return *(m_cvars.insert(new SConPropString(name, help, flags, methods, default_value)).first);
	}
	return nullptr;
}



///-----
void Console::set_wait_seconds(float value) {
	wait_seconds = value > 0 ? value : 0;
	godot::Godot::print("waiting...");
}
float Console::get_wait_seconds() {
	return wait_seconds;
}