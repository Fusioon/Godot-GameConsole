#include <Godot.hpp>
#include <Sprite.hpp>

#include "FusionConsole.hpp"
#include "ConsoleBindings.hpp"

fusion::Console* fusion::pConsole = nullptr;

void im_helpless()
{
	godot::Godot::print("FUCKYOU");
}

namespace godot {

	class gdexample : public Sprite {
		GODOT_CLASS(gdexample, Sprite)

	private:
		float time_passed;
		float speed;
		Vector2 start_pos;
	public:
		static void _register_methods();

		gdexample() : time_passed(0) {}
		~gdexample();

		void _init();
		void _process(float delta);
		void print_text(String text);
	};
	
	void gdexample::_register_methods() {
		register_method("_init", &gdexample::_init);
		register_method("_process", &gdexample::_process);
		register_method("print_text", &gdexample::print_text);
	}

	gdexample::~gdexample() {
		// add your cleanup here
	}

	void gdexample::_init() {
		time_passed = 0.0;
		speed = 2;
		start_pos = get_position();
		fusion::pConsole->RegisterCVar("speed", "speed of the sprite", ECVarFlags::Null, &speed, 0);
		fusion::pConsole->RegisterCommand("helpless", "im am helpless", ECVarFlags::Null, make_command<im_helpless>());
		fusion::pConsole->EnqueueCommand("wait.seconds 1; speed 3; helpless");
	}

	void gdexample::_process(float delta) {
		time_passed += delta;
		
		Vector2 new_position = start_pos + Vector2(100.0f + (70 * speed * sin(time_passed * 2)), 35.0f + (70.0f * speed * cos(time_passed * 4)));
		set_position(new_position);
	}
	void gdexample::print_text(String text)
	{
		
		Godot::print(text);
	}
}

using fusion::Console;

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options* o) {
#define MAKE_COMMAND(TargetType, Method, ThisPtr) make_command<TargetType, &TargetType::Method>(ThisPtr)
	godot::Godot::gdnative_init(o);
	
	fusion::pConsole = new fusion::Console();
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* o) {

	godot::Godot::gdnative_terminate(o);

	delete fusion::pConsole;
	fusion::pConsole = nullptr;
}

extern "C" void GDN_EXPORT godot_nativescript_init(void* handle) {
	godot::Godot::nativescript_init(handle);
	godot::register_class<godot::gdexample>();
	godot::register_class<godot::ConsoleBindings>();
}
