#include "ConsoleUI.hpp"
#include "FusionConsole.hpp"
#include "CanvasLayer.hpp"
#include "Input.hpp"
#include "InputEventKey.hpp"
#include "SceneTree.hpp"
using namespace godot;
#include "InputMap.hpp"


#define REGISTER_METHOD(obj, method) register_method(#method, &obj::method)
void ConsoleUI::_register_methods()
{
	REGISTER_METHOD(ConsoleUI, _init);
	REGISTER_METHOD(ConsoleUI, _ready);
	REGISTER_METHOD(ConsoleUI, _on_change);
	REGISTER_METHOD(ConsoleUI, _on_submit);
	REGISTER_METHOD(ConsoleUI, _on_focus_lost);
	REGISTER_METHOD(ConsoleUI, _input);
	/*
	register_property<ConsoleUI, LineEdit*>("command_line", &ConsoleUI::m_cmdline, nullptr);
	register_property<ConsoleUI, RichTextLabel*>("suggestions", &ConsoleUI::m_suggestions, nullptr);
	register_property<ConsoleUI, RichTextLabel*>("output", &ConsoleUI::m_output, nullptr);
	*/
	/*register_property<ConsoleUI, LineEdit*>("commandline_input", &ConsoleUI::m_cmdline, nullptr);
	register_property<ConsoleUI, RichTextLabel*>("suggestions_output", &ConsoleUI::m_suggestions, nullptr);
	register_property<ConsoleUI, RichTextLabel*>("console_output", &ConsoleUI::m_output, nullptr);*/
}

void ConsoleUI::_init() {
	
	m_ignore_change = false;
	m_autocomplete_notempty = false;
	m_log_colors[(int)ELogType::Null] = Color(1, 1, 1, 1);
	m_log_prefix[(int)ELogType::Null] = "";
	m_log_colors[(int)ELogType::Success] = Color(0, 1, 0, 1);
	m_log_prefix[(int)ELogType::Success] = "[Success] ";
	m_log_colors[(int)ELogType::Warning] = Color(1, 1, 0, 1);
	m_log_prefix[(int)ELogType::Warning] = "[Warning] ";
	m_log_colors[(int)ELogType::Error] = Color(1, 0, 0, 1);
	m_log_prefix[(int)ELogType::Error] = "[Error] ";
	m_log_colors[(int)ELogType::Exception] = Color(1, 0, 1, 1);
	m_log_prefix[(int)ELogType::Exception] = "[Exception] ";


}


void ConsoleUI::_ready() {

	fusion::pConsole->SetUI(this);

	auto try_find = [this](Node* child) -> bool {
		auto name = child->get_name();
		if (!m_cmdline && name.find("input") >= 0) {
			m_cmdline = cast_to<LineEdit>(child);
			return true;
		}
		if (!m_suggestions && name.find("suggest") >= 0) {
			m_suggestions = cast_to<RichTextLabel>(child);
			return true;
		}
		if (!m_output && name.find("output") >= 0) {
			m_output = cast_to<RichTextLabel>(child);
			return true;
		}
		return false;
	};

	for (int64_t i = 0; i < get_child_count(); ++i) {
		auto child = get_child(i);
		if (try_find(child)) continue;
		for (int64_t j = 0; j < child->get_child_count(); ++j) {
			auto sub_child = child->get_child(j);
			try_find(sub_child);
		}
	}

	if (!m_output) {
		Godot::print_error("Failed to find output RichTextLabel in ConsoleUI", __func__, __FILE__, __LINE__);
	}

	if (!m_suggestions) {
		Godot::print_error("Failed to find suggestions RichTextLabel in ConsoleUI", __func__, __FILE__, __LINE__);
	}
	

	if (!m_cmdline) {
		Godot::print_error("Failed to find input LineEdit in ConsoleUI", __func__, __FILE__, __LINE__);
	}

	if (m_cmdline) {
		m_cmdline->set_text("");
		m_cmdline->connect("focus_exited", this, "_on_focus_lost");
		m_cmdline->connect("text_changed", this, "_on_change");
		m_cmdline->connect("text_entered", this, "_on_submit");
		m_cmdline->grab_focus();
	}

	if (m_suggestions) {
		m_suggestions_rect = Object::cast_to<ColorRect>(m_suggestions->get_parent());
		m_suggestions_rect->hide();
	}

	if (m_output) {
		m_output->set_scroll_follow(true);
	}
	
}

void ConsoleUI::_process() {

}

void ConsoleUI::_input(const Variant& v) {
	auto input = Input::get_singleton();
	enum {
		KEY_TAB = 16777218,
		KEY_UP = 16777232,
		KEY_DOWN = 16777234,
		KEY_SHIFT = 16777237,
	};
	
	if (input->is_key_pressed(KEY_TAB)) {
		get_tree()->set_input_as_handled();
		

		auto found_autocomplete = [this](ICVar* pCvar) -> void {
			m_ignore_change = true;
			m_cmdline->set_text(pCvar->GetName() + " ");
			m_cmdline->set_cursor_position(999);
			m_cmdline->_text_changed();
		};

		if (input->is_key_pressed(KEY_SHIFT)) {
			
		}
		else {
			/*for (ICVar* pCvar = m_autocomplete_iter.current(); 
				(pCvar = m_autocomplete_iter.next()) || 
				(m_autocomplete_notempty && (pCvar = m_autocomplete_iter.next()))
				;) {
				if (pCvar->GetName().begins_with(m_autocomplete_iter.name())) {
					m_autocomplete_iter.next();
					m_autocomplete_notempty = true;
					found_autocomplete(pCvar);
					break;
				}
			}*/
			
		}
	}

	if (input->is_key_pressed(KEY_UP)) {
		get_tree()->set_input_as_handled();
		auto next = fusion::pConsole->GetHistory().up();
		m_cmdline->set_text(next);
		m_cmdline->_text_changed();
		m_cmdline->set_cursor_position(next.length());
	}
	if (input->is_key_pressed(KEY_DOWN)) {
		get_tree()->set_input_as_handled();
		auto next = fusion::pConsole->GetHistory().down();
		m_cmdline->set_text(next);
		m_cmdline->_text_changed();
		m_cmdline->set_cursor_position(next.length());
	}
}

void ConsoleUI::_on_change(String new_value) {
	constexpr auto k_MaxSuggestions = 7;

	if (!m_ignore_change) {
		//m_autocomplete_iter = fusion::pConsole->GetAutocomplete(new_value);
		m_autocomplete_notempty = false;
	}
	m_ignore_change = false;

	if (m_last_value == new_value) {
		return;
	}

	m_last_value = new_value;

	if (new_value.empty()) {
		m_suggestions_rect->hide();
		return;
	}

	auto print_suggestion = [this](ICVar* pCvar) -> void {
		auto name = pCvar->GetName();
		auto help = pCvar->GetHelp();
		
		m_suggestions->push_color(Color(1, 1, 0, 1));
		m_suggestions->add_text(name);
		m_suggestions->pop();
		if (!help.empty()) {
			m_suggestions->add_text(" - " + help);
		}
		m_suggestions->add_text("\n");
	};

	m_suggestions->clear();
	
	ICVar* sug[k_MaxSuggestions];
	size_t count = fusion::pConsole->GetSuggestions(new_value, sug, k_MaxSuggestions);
	for (size_t i = 0; i < count; ++i) {
		print_suggestion(sug[i]);
	}
	
	if (count > 0) {
		m_suggestions_rect->show();
	}
	else {
		m_suggestions_rect->hide();
	}
}

void ConsoleUI::_on_submit(String new_value) {
	if (new_value.length() > 0) {
		m_cmdline->set_text(""); 
		m_cmdline->_text_changed();
		fusion::pConsole->EnqueueCommand(new_value);
	}
}

void ConsoleUI::_on_focus_lost() {
	if (is_visible()) {
		auto c_pos = m_cmdline->get_cursor_position();
		m_cmdline->grab_focus();
		m_cmdline->set_cursor_position(c_pos);
	}
}

void ConsoleUI::Open() {
	show();
	m_cmdline->grab_focus();
}

void ConsoleUI::Close() {
	hide();
}

void ConsoleUI::PrintLine(const String& line, ELogType log_type) {

	m_output->push_color(m_log_colors[(int)log_type]);
	m_output->add_text("\n"+ m_log_prefix[(int)log_type] + line);
	m_output->pop();
}

void ConsoleUI::Clear() {
	m_output->clear();
}
