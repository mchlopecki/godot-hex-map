#include "tiled_node/tiled_node.h"
#ifdef TOOLS_ENABLED

#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/mesh_library.hpp>

#include "../int_node.h"
#include "core/editor/editor_plugin.h"

using namespace godot;

// XXX move palette to bottom panel for HexGrid
class HexMapIntNodeEditorPlugin : public HexMapNodeEditorPlugin {
    GDCLASS(HexMapIntNodeEditorPlugin, HexMapNodeEditorPlugin);

public:
    virtual bool _handles(Object *object) const override;
    virtual void _edit(Object *p_object) override;
    // virtual int32_t _forward_3d_gui_input(Camera3D *viewport_camera,
    //         const Ref<InputEvent> &event) override;

    HexMapIntNodeEditorPlugin() {};
    ~HexMapIntNodeEditorPlugin() {};

protected:
    // void _notification(int p_what);
    static void _bind_methods() {};

private:
    HexMapIntNode *int_map = nullptr;
    HexMapTiledNode *tile_node = nullptr;
    Control *bottom_panel = nullptr;
    Ref<MeshLibrary> mesh_library;

    /// update the `MeshLibrary` used in the `HexMapTiledNode` editor display
    void update_mesh_library();
};

#endif
