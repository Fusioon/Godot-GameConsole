#include "ConsoleBindings.hpp"
#include "FusionConsole.hpp"

using namespace godot;


void ConsoleBindings::_register_methods()
{
	godot::register_method("_init", &ConsoleBindings::_init);
	godot::register_method("_process", &ConsoleBindings::_process);
	godot::register_method("enqueue_command", &ConsoleBindings::EnqueueCommand);
}

void ConsoleBindings::_init()
{
	fusion::pConsole->EnqueueCommand("wait.seconds 1; speed 3; helpless");
}

void ConsoleBindings::_process(float dt)
{
	fusion::pConsole->Update(dt);
}

void ConsoleBindings::EnqueueCommand(const godot::String& cmd_line) const
{
	fusion::pConsole->EnqueueCommand(cmd_line);
}