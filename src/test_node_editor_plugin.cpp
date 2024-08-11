#include "test_node_editor_plugin.h"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/core/error_macros.hpp"

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

TestNodeEditorPlugin::TestNodeEditorPlugin() {}

TestNodeEditorPlugin::~TestNodeEditorPlugin() {}
