#include "ConsoleBindings.hpp"
#include "FusionConsole.hpp"

using namespace godot;



void ConsoleBindings::_register_methods()
{
	godot::register_method("_init", &ConsoleBindings::_init);
	godot::register_method("_process", &ConsoleBindings::_process);
}

void ConsoleBindings::_init()
{
	
}

void ConsoleBindings::_process(float dt)
{
	fusion::pConsole->Update(dt);
}