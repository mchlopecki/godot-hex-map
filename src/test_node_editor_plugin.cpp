#include "test_node_editor_plugin.h"
#include "godot_cpp/classes/cylinder_mesh.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/shortcut.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "hex_map/hex_map_cell_id.h"
#include "test_node.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

bool TestNodeEditorPlugin::_handles(Object *p_object) const {
	return p_object->is_class("TestNode");
}

void TestNodeEditorPlugin::_edit(Object *p_object) {
	if (p_object != nullptr) {
		UtilityFunctions::print("p_object " + p_object->get_class());
		TestNode *test_node = Object::cast_to<TestNode>(p_object);
		ERR_FAIL_NULL(test_node);
	}
}

Ref<Shortcut> TestNodeEditorPlugin::editor_shortcut(const String &p_path,
		const String &p_name, Key p_keycode, bool p_physical) {
	Ref<EditorSettings> editor_settings =
			get_editor_interface()->get_editor_settings();

	Array events;
	Ref<InputEventKey> input_event_key;
	if (p_keycode != KEY_NONE) {
		input_event_key.instantiate();
		if (p_physical) {
			input_event_key->set_physical_keycode(
					(Key)(p_keycode & KEY_CODE_MASK));
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

TestNodeEditorPlugin::TestNodeEditorPlugin() {
	editor_shortcut("test_node/test_setting", "TestNode Test Setting",
			(Key)(KEY_C | godot::KEY_CTRL), true);
}

TestNodeEditorPlugin::~TestNodeEditorPlugin() {}
