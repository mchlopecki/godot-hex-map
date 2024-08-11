#pragma once

#include <godot_cpp/classes/editor_plugin.hpp>

#include "test_node.h"

using namespace godot;

class TestNodeEditorPlugin : public EditorPlugin {
	GDCLASS(TestNodeEditorPlugin, EditorPlugin)

private:
	TestNode *node = nullptr;

protected:
	static void _bind_methods() {};

public:
	virtual bool _handles(Object *object) const override;
	virtual void _edit(Object *p_object) override;

	TestNodeEditorPlugin();
	~TestNodeEditorPlugin();
};
