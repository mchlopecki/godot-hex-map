#include "godot_cpp/classes/packed_scene.hpp"
#ifdef TOOLS_ENABLED

#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/mesh_library.hpp>

#include "auto_tiled_node/auto_tiled_node.h"

using namespace godot;

class HexMapAutoTiledNodeEditorPlugin : public EditorPlugin {
    GDCLASS(HexMapAutoTiledNodeEditorPlugin, EditorPlugin);

public:
    virtual bool _handles(Object *object) const override;
    virtual void _edit(Object *p_object) override;

    HexMapAutoTiledNodeEditorPlugin();
    ~HexMapAutoTiledNodeEditorPlugin() {};

protected:
    static void _bind_methods() {};

private:
    // signal handler callbacks
    void on_mesh_library_changed();

    /// HexMapAutoTiledNode being edited
    HexMapAutoTiledNode *auto_tiled_node = nullptr;

    /// scene for the bottom panel if one is defined
    Ref<PackedScene> bottom_panel_scene;

    /// gui bottom panel holding all user controls for this plugin
    Control *bottom_panel = nullptr;
};

#endif
