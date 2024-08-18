#include "editor_control.h"

#include "godot_cpp/classes/box_container.hpp"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_settings.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/popup_menu.hpp"
#include "godot_cpp/classes/shortcut.hpp"
#include "godot_cpp/classes/theme.hpp"
#include "godot_cpp/classes/v_separator.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

Ref<Shortcut>
EditorControl::editor_shortcut(const String &p_path, const String &p_name, Key p_keycode, bool p_physical) {
	Ref<EditorSettings> editor_settings =
			EditorInterface::get_singleton()->get_editor_settings();

	Array events;
	Ref<InputEventKey> input_event_key;
	if (p_keycode != KEY_NONE) {
		input_event_key.instantiate();
		if (p_physical) {
			input_event_key->set_physical_keycode((Key)(p_keycode & KEY_CODE_MASK));
		} else {
			input_event_key->set_keycode((Key)(p_keycode & KEY_CODE_MASK));
		}
		if ((p_keycode & KEY_MASK_SHIFT) != 0) {
			input_event_key->set_shift_pressed(true);
		}
		if ((p_keycode & KEY_MASK_ALT) != 0) {
			input_event_key->set_alt_pressed(true);
		}
		if ((p_keycode & KEY_MASK_CMD_OR_CTRL) != 0) {
			input_event_key->set_command_or_control_autoremap(true);
		} else if ((p_keycode & KEY_MASK_CTRL) != 0) {
			input_event_key->set_ctrl_pressed(true);
		} else if ((p_keycode & KEY_MASK_META) != 0) {
			input_event_key->set_meta_pressed(true);
		}
		events.push_back(input_event_key);
	}

	Ref<Shortcut> shortcut = editor_settings->get_setting(p_path);
	if (shortcut.is_valid()) {
		shortcut->set_name(p_name);
		shortcut->set_meta("original", events.duplicate(true));
	} else {
		shortcut.instantiate();
		shortcut->set_name(p_name);
		shortcut->set_events(events);
		shortcut->set_meta("original", events.duplicate(true));
		editor_settings->set_setting(p_path, shortcut);
	}

	return shortcut;
}

void EditorControl::handle_action(int p_action) {
	UtilityFunctions::print("handle_action() " + itos(p_action));

	switch ((Action)p_action) {
		case PLANE_DOWN:
			plane_spin_box->set_value(plane_spin_box->get_value() - 1);
			break;
		case PLANE_UP:
			plane_spin_box->set_value(plane_spin_box->get_value() + 1);
			break;
	}
}

void EditorControl::set_plane(int p_plane) {
	plane[active_axis] = p_plane;
	emit_signal("plane_changed", active_axis, p_plane);
}

void EditorControl::_bind_methods() {
	ADD_SIGNAL(MethodInfo("plane_changed",
			PropertyInfo(Variant::INT, "axis"),
			PropertyInfo(Variant::INT, "plane")));
}

bool EditorControl::handle_keypress(Ref<InputEventKey> p_event) {
	ERR_FAIL_COND_V(!p_event.is_valid() || !p_event->is_pressed(), false);

	PopupMenu *popup = menu_button->get_popup();
	int count = popup->get_item_count();

	for (int i = 0; i < count; i++) {
		const Ref<Shortcut> shortcut = popup->get_item_shortcut(i);
		if (shortcut.is_valid() && shortcut->matches_event(p_event)) {
			handle_action(popup->get_item_id(i));
			return true;
		}
	}
	return false;
}

EditorControl::EditorControl() {
	EditorInterface *editor_interface = EditorInterface::get_singleton();
	Ref<Theme> theme = editor_interface->get_editor_theme();

	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_alignment(BoxContainer::ALIGNMENT_END);

	plane_label = memnew(Label);
	plane_label->set_text("Floor:");
	add_child(plane_label);

	plane_spin_box = memnew(SpinBox);
	plane_spin_box->set_min(SHRT_MIN);
	plane_spin_box->set_max(SHRT_MAX);
	plane_spin_box->set_step(1);
	plane_spin_box->get_line_edit()->add_theme_constant_override(
			"minimum_character_width", 8);
	plane_spin_box->connect("value_changed", callable_mp(this, &EditorControl::set_plane));
	add_child(plane_spin_box);

	add_child(memnew(VSeparator));

	menu_button = memnew(MenuButton);
	menu_button->set_text("Hex Map");
	menu_button->set_button_icon(theme->get_icon("GridMap", "EditorIcons"));
	add_child(menu_button);

	PopupMenu *popup = menu_button->get_popup();
	popup->connect("id_pressed",
			callable_mp(this, &EditorControl::handle_action));

	popup->add_shortcut(editor_shortcut("hex_map/previous_floor",
								"Previous Floor",
								Key::KEY_Q, true),
			Action::PLANE_DOWN);
	popup->add_shortcut(editor_shortcut("hex_map/next_floor",
								"Next Floor",
								Key::KEY_E, true),
			Action::PLANE_UP);
}

EditorControl::~EditorControl() {
	memfree(menu_button);
	memfree(plane_spin_box);
	memfree(plane_label);
}
