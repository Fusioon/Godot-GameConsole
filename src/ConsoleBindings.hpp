#pragma once

#include "Godot.hpp"
#include "Node.hpp"

namespace godot {
	class ConsoleBindings : public Node
	{
		GODOT_CLASS(ConsoleBindings, Node);

	public:
		static void _register_methods();
		void _init();
		void _process(float dt);

	};
}



