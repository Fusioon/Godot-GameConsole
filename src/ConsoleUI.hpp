#pragma once

#include "Godot.hpp"
#include "LineEdit.hpp"
#include "RichTextLabel.hpp"
#include "Ref.hpp"
#include "IConsole.h"
#include "ColorRect.hpp"
#include "FusionConsole.hpp"

namespace godot {

	class ConsoleUI : public Control, public IConsoleUI {
		GODOT_CLASS(ConsoleUI, Control);

		RichTextLabel* m_output;
		RichTextLabel* m_suggestions;
		LineEdit* m_cmdline;

		ColorRect* m_suggestions_rect;

		Color m_log_colors[(size_t)ELogType::MAX];
		String m_log_prefix[(size_t)ELogType::MAX];

		fusion::Console::Autocomplete m_autocomplete_iter;
		bool m_autocomplete_notempty;
		godot::String m_last_value;
		bool m_ignore_change;
	public:
		static void _register_methods();

		void _init();
		void _ready();
		void _process();
		void _input(const Variant& v);
		void _on_change(String new_value);
		void _on_submit(String new_value);
		void _on_focus_lost();


		virtual void Open();
		virtual void Close();
		virtual void PrintLine(const String& line, ELogType log_type);
		virtual void Clear();
	};

}