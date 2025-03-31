#include "core/space.h"
#include "godot_cpp/classes/editor_undo_redo_manager.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "tiled_node/tiled_node.h"
#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "editor_plugin.h"

bool HexMapIntNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapInt");
}

void HexMapIntNodeEditorPlugin::_edit(Object *p_object) {
    HexMapNodeEditorPlugin::_edit(p_object);

    if (hex_map) {
        // get IntNode properly casted
        int_node = Object::cast_to<HexMapIntNode>(p_object);

        // XXX Move this loading to the constructor when gui stablizes
        // load the bottom panel scene and add it to the editor
        Ref<PackedScene> panel_scene = ResourceLoader::get_singleton()->load(
                "res://addons/hexmap/gui/"
                "int_node_editor_bottom_panel.tscn");
        bottom_panel = (Control *)panel_scene->instantiate();

        // connect the signals
        bottom_panel->connect("type_changed",
                callable_mp(this, &HexMapIntNodeEditorPlugin::rebuild_cursor));
        bottom_panel->connect("update_type",
                callable_mp(this, &HexMapIntNodeEditorPlugin::set_cell_type));
        bottom_panel->connect("delete_type",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::delete_cell_type));
        bottom_panel->connect("edit_plane_changed",
                callable_mp(this, &HexMapIntNodeEditorPlugin::set_edit_plane));

        // set known state
        bottom_panel->set("cell_types", int_node->_get_cell_types());
        bottom_panel->set("edit_plane_axis", (int)edit_axis);
        bottom_panel->set("edit_plane_depth", edit_axis_depth[edit_axis]);

        // add & show the panel
        add_control_to_bottom_panel(bottom_panel, "HexMapInt");
        make_bottom_panel_item_visible(bottom_panel);

        // create a TiledNode to visualize the contents of the IntNode
        tile_node = memnew(HexMapTiledNode);
        tile_node->set_space(int_node->get_space());
        // the cell meshes we create in update_mesh_library() are centered on
        // origin, so set the TileNode to center the mesh in the cell.
        tile_node->set_center_y(true);
        update_mesh_library();

        // XXX editor_cursor is getting hung up on center-y, when set to false
        // on the IntNode editor_cursor doesn't line up with the real grid.

        // populate the tiled node with our cells
        for (const auto &iter : int_node->cell_map) {
            tile_node->set_cell(iter.key, iter.value);
        }
        add_child(tile_node);

        editor_cursor->set_cells_visibility_callback(callable_mp(
                tile_node, &HexMapTiledNode::set_cells_visibility_callback));

        int_node->connect("editor_plugin_cell_changed",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::update_tiled_node));
        int_node->connect("editor_plugin_types_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::update_mesh_library));

    } else {
        int_node->disconnect("editor_plugin_cell_changed",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::update_tiled_node));
        int_node->disconnect("editor_plugin_types_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::update_mesh_library));
        int_node = nullptr;

        if (tile_node != nullptr) {
            remove_child(tile_node);
            memfree(tile_node);
            tile_node = nullptr;
        }

        if (bottom_panel != nullptr) {
            remove_control_from_bottom_panel(bottom_panel);
            memfree(bottom_panel);
            bottom_panel = nullptr;
        }

        set_process(false);
        return;
    }
}

void HexMapIntNodeEditorPlugin::update_mesh_library() {
    mesh_library.instantiate();
    const HexMapSpace &space = int_node->get_space();

    for (const auto &iter : int_node->get_cell_types()) {
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_albedo(iter.value.color);
        if (iter.value.color.a < 1.0) {
            material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
        }
        //material->set_shading_mode(
        //        StandardMaterial3D::SHADING_MODE_PER_VERTEX);
        // material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
        Ref<StandardMaterial3D> line_material;
        line_material.instantiate();
        line_material->set_albedo(iter.value.color.inverted());
        line_material->set_shading_mode(
                StandardMaterial3D::SHADING_MODE_UNSHADED);
        line_material->set_transparency(
                StandardMaterial3D::TRANSPARENCY_ALPHA);
        line_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

        Ref<Mesh> mesh = space.build_cell_mesh();
        mesh->surface_set_material(0, material);
        mesh->surface_set_material(1, line_material);
        mesh_library->create_item(iter.key);
        mesh_library->set_item_name(iter.key, iter.value.name);
        mesh_library->set_item_mesh(iter.key, mesh);
    }

    if (tile_node) {
        tile_node->set_mesh_library(mesh_library);
    }

    if (editor_cursor) {
        editor_cursor->set_mesh_library(mesh_library);
        rebuild_cursor();
    }

    if (bottom_panel) {
        bottom_panel->set("cell_types", int_node->_get_cell_types());
    }
}

void HexMapIntNodeEditorPlugin::update_tiled_node(Vector3i cell_id_v,
        int type) {
    ERR_FAIL_COND_MSG(tile_node == nullptr, "tiled node not allocated");
    tile_node->set_cell(cell_id_v, type);
}

void HexMapIntNodeEditorPlugin::rebuild_cursor() {
    ERR_FAIL_COND(bottom_panel == nullptr);
    int type = bottom_panel->get("selected_type");
    editor_cursor->clear_tiles();
    if (type >= 0) {
        editor_cursor->set_tile(HexMapCellId(), type);
    }
}

void HexMapIntNodeEditorPlugin::set_cell_type(int id,
        String name,
        Color color) {
    ERR_FAIL_COND(int_node == nullptr);
    ERR_FAIL_COND(bottom_panel == nullptr);

    // work-around for dynamic assignment; if they didn't provide an id,
    // make the change directly now to get the id.  The do_method will just
    // repeat the same action with the fixed id we get here.
    //
    // We need a valid id here to be able to add the undo method.
    if (id == HexMapIntNode::TypeIdNext) {
        id = int_node->set_cell_type(id, name, color);
    }

    EditorUndoRedoManager *undo_redo = get_undo_redo();
    undo_redo->create_action("HexMapInt: set cell type");
    undo_redo->add_do_method(int_node, "set_cell_type", id, name, color);
    undo_redo->add_undo_method(int_node, "remove_cell_type", id); // no ID
    undo_redo->commit_action();
}

void HexMapIntNodeEditorPlugin::delete_cell_type(int id) {
    ERR_FAIL_COND_MSG(int_node == nullptr, "int_node not set");

    // look up the name & color for the cell type; if it doesn't exist, nothing
    // to be done here.
    const auto cell_type = int_node->cell_types.find(id);
    if (cell_type == int_node->cell_types.end()) {
        return;
    }

    // find the cells in the map to delete
    Vector<HexMapCellId::Key> cells;
    for (auto &iter : int_node->cell_map) {
        if (iter.value == id) {
            cells.push_back(iter.key);
        }
    }

    // create do and undo arrays for deleting the cells with this value
    Array do_list, undo_list;
    do_list.resize(cells.size() * HexMapNode::CellArrayWidth);
    undo_list.resize(cells.size() * HexMapNode::CellArrayWidth);
    int count = 0;
    for (const auto key : cells) {
        int base = count * HexMapNode::CellArrayWidth;
        Vector3i cell = static_cast<Vector3i>(key);
        do_list[base] = cell;
        do_list[base + 1] = HexMapNode::INVALID_CELL_ITEM;
        undo_list[base] = cell;
        undo_list[base + 1] = id;
        count++;
    }

    EditorUndoRedoManager *undo_redo = get_undo_redo();
    undo_redo->create_action("HexMapInt: remove cell type");
    undo_redo->add_do_method(int_node, "set_cells", do_list);
    undo_redo->add_do_method(int_node, "remove_cell_type", id);
    undo_redo->add_undo_method(int_node,
            "set_cell_type",
            id,
            cell_type->value.name,
            cell_type->value.color);
    undo_redo->add_undo_method(int_node, "set_cells", undo_list);
    undo_redo->commit_action();
}

void HexMapIntNodeEditorPlugin::set_edit_plane(int axis, int depth) {
    if (edit_axis != axis) {
        edit_plane_set_axis((EditorCursor::EditAxis)axis);
    } else {
        edit_plane_set_depth(depth);
    }
}
#endif // TOOLS_ENABLED
