#ifdef TOOLS_ENABLED

#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/mesh_library.hpp>

#include "core/editor/editor_plugin.h"
#include "int_node/int_node.h"
#include "tiled_node/tiled_node.h"

using namespace godot;

class HexMapIntNodeEditorPlugin : public HexMapNodeEditorPlugin {
    GDCLASS(HexMapIntNodeEditorPlugin, HexMapNodeEditorPlugin);

public:
    virtual bool _handles(Object *object) const override;
    virtual void _edit(Object *p_object) override;

    HexMapIntNodeEditorPlugin();
    ~HexMapIntNodeEditorPlugin() {};

protected:
    static void _bind_methods();

private:
    /// HexMapIntNode being edited
    HexMapIntNode *int_node = nullptr;

    /// HexMapTiledNode used to render the contents of `int_node`
    HexMapTiledNode *tiled_node = nullptr;

    /// Generated MeshLibrary used in `tile_node`, contains hex cells with
    /// colors defined by `int_node->get_cell_types()`.
    Ref<MeshLibrary> mesh_library;

    /// update the `MeshLibrary` used in the `HexMapTiledNode` editor display
    void on_int_node_cell_types_changed();

    /// update the `HexMapTiledNode` based on a cell change
    void on_int_node_cells_changed(Array cells);

    void on_int_node_hex_space_changed();

    /// set/get visibility of int node cells
    void set_cells_visible(bool value);
    bool get_cells_visible() const;

    /// callback from editor_plugin to set cell visibility
    void set_cells_visibility(const Array);
};

#endif
