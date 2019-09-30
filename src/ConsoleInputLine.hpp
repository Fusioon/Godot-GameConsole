#pragma once

#include "Godot.hpp"
#include "LineEdit.hpp"
#include "RichTextLabel.hpp"

namespace godot {

	class ConsoleUI : public LineEdit {
		GODOT_CLASS(ConsoleUI, LineEdit);

		String last_value;
		Ref<RichTextLabel> m_output;
		Ref<RichTextLabel> m_suggestions;
		Ref<LineEdit> m_cmdline;
	public:
		static void _register_methods();

		void _init();
		void _process();
		void _on_change(String new_value);
		void _on_submit(String new_value);
		void _on_focus_lost();
	};

}