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
	TestObject o(1);
	return p_object->is_class("TestNode");
}

void TestNodeEditorPlugin::_edit(Object *p_object) {
	if (p_object != nullptr) {
		UtilityFunctions::print("p_object " + p_object->get_class());
		TestNode *test_node = Object::cast_to<TestNode>(p_object);
		ERR_FAIL_NULL(test_node);
	}
}

// Ref<Shortcut> ED_SHORTCUT_ARRAY(const String &p_path, const String &p_name,
// const PackedInt32Array &p_keycodes, bool p_physical) { 	Array events;
//
// 	for (int i = 0; i < p_keycodes.size(); i++) {
// 		Key keycode = (Key)p_keycodes[i];
//
// 		Ref<InputEventKey> ie;
// 		if (keycode != Key::NONE) {
// 			ie = InputEventKey::create_reference(keycode, p_physical);
// 			events.push_back(ie);
// 		}
// 	}
//
// 	if (!EditorSettings::get_singleton()) {
// 		Ref<Shortcut> sc;
// 		sc.instantiate();
// 		sc->set_name(p_name);
// 		sc->set_events(events);
// 		sc->set_meta("original", events.duplicate(true));
// 		return sc;
// 	}
//
// 	Ref<Shortcut> sc = EditorSettings::get_singleton()->get_shortcut(p_path);
// 	if (sc.is_valid()) {
// 		sc->set_name(p_name); //keep name (the ones that come from disk have no
// name) 		sc->set_meta("original", events.duplicate(true)); //to compare
// against changes 		return sc;
// 	}
//
// 	sc.instantiate();
// 	sc->set_name(p_name);
// 	sc->set_events(events);
// 	sc->set_meta("original", events.duplicate(true)); //to compare against
// changes 	EditorSettings::get_singleton()->_add_shortcut_default(p_path, sc);
//
// 	return sc;
// }
//
// Ref<Shortcut> ED_SHORTCUT(const String &p_path, const String &p_name, Key
// p_keycode, bool p_physical) { 	PackedInt32Array arr;
// 	arr.push_back((int32_t)p_keycode);
// 	return ED_SHORTCUT_ARRAY(p_path, p_name, arr, p_physical);
// }

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
