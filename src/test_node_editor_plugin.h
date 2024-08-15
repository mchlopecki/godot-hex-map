#pragma once

#include <godot_cpp/classes/editor_plugin.hpp>

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/shortcut.hpp"
#include "test_node.h"

using namespace godot;

class TestNodeEditorPlugin : public EditorPlugin {
	GDCLASS(TestNodeEditorPlugin, EditorPlugin)

private:
	TestNode *node = nullptr;

protected:
	static void _bind_methods() {};
	Ref<Shortcut> editor_shortcut(const String &p_path, const String &p_name, Key p_keycode, bool p_physical);

public:
	virtual bool _handles(Object *object) const override;
	virtual void _edit(Object *p_object) override;

	TestNodeEditorPlugin();
	~TestNodeEditorPlugin();
};
