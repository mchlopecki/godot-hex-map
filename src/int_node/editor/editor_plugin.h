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

    HexMapIntNodeEditorPlugin() {};
    ~HexMapIntNodeEditorPlugin() {};

protected:
    static void _bind_methods() {};

private:
    /// HexMapIntNode being edited
    HexMapIntNode *int_node = nullptr;

    /// HexMapTiledNode used to render the contents of `int_node`
    HexMapTiledNode *tiled_node = nullptr;

    /// Generated MeshLibrary used in `tile_node`, contains hex cells with
    /// colors defined by `int_node->get_cell_types()`.
    Ref<MeshLibrary> mesh_library;

    /// update the `MeshLibrary` used in the `HexMapTiledNode` editor display
    void update_mesh_library();

    /// update the `HexMapTiledNode` based on a cell change
    void on_int_node_cells_changed(Array cells);

    // signal handlers
    void cell_scale_changed();
    void set_cell_type(int id, String name, Color color);
    void delete_cell_type(int id);
    void set_edit_plane(int axis, int depth);
    void set_tiled_map_visibility(bool visible);
};

#endif
