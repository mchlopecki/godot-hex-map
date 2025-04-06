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
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::rebuild_cursor));
        bottom_panel->connect("update_type",
                callable_mp(this, &HexMapIntNodeEditorPlugin::set_cell_type));
        bottom_panel->connect("delete_type",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::delete_cell_type));
        bottom_panel->connect("edit_plane_changed",
                callable_mp(this, &HexMapIntNodeEditorPlugin::set_edit_plane));
        bottom_panel->connect("rotate_cursor",
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::cursor_rotate));
        bottom_panel->connect("move_selection",
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::selection_move));
        bottom_panel->connect("clone_selection",
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::selection_clone));
        bottom_panel->connect("fill_selection",
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::selection_fill));
        bottom_panel->connect("clear_selection",
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::selection_clear));
        bottom_panel->connect("set_tiled_map_visibility",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::set_tiled_map_visibility));

        // set known state
        bottom_panel->set("cell_types", int_node->_get_cell_types());
        bottom_panel->set("edit_plane_axis", (int)edit_axis);
        bottom_panel->set("edit_plane_depth", edit_axis_depth[edit_axis]);
        bool show_cells = int_node->get_meta("_show_cells_", true);
        bottom_panel->set("show_cells", show_cells);

        // add & show the panel
        add_control_to_bottom_panel(bottom_panel, "HexMapInt");
        make_bottom_panel_item_visible(bottom_panel);

        // create a TiledNode to visualize the contents of the IntNode
        tiled_node = memnew(HexMapTiledNode);
        tiled_node->set_space(int_node->get_space());
        // the cell meshes we create in update_mesh_library() are centered on
        // origin, so set the TileNode to center the mesh in the cell.
        tiled_node->set_center_y(true);
        tiled_node->set_visible(show_cells);
        update_mesh_library();

        // XXX editor_cursor is getting hung up on center-y, when set to false
        // on the IntNode editor_cursor doesn't line up with the real grid.

        // populate the tiled node with our cells
        for (const auto &iter : int_node->cell_map) {
            tiled_node->set_cell(iter.key, iter.value);
        }

        // add the tiled node to the int_node to inherit the visibility of the
        // int node.
        int_node->add_child(tiled_node);

        editor_cursor->set_cells_visibility_callback(callable_mp(
                tiled_node, &HexMapTiledNode::set_cells_visibility_callback));

        int_node->connect("cell_scale_changed",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::cell_scale_changed));
        int_node->connect("editor_plugin_cell_changed",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::update_tiled_node));
        int_node->connect("editor_plugin_types_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::update_mesh_library));

    } else {
        if (tiled_node != nullptr) {
            int_node->set_meta("_show_cells_", tiled_node->is_visible());
            int_node->remove_child(tiled_node);
            memfree(tiled_node);
            tiled_node = nullptr;
        }
        int_node->disconnect("cell_scale_changed",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::cell_scale_changed));
        int_node->disconnect("editor_plugin_cell_changed",
                callable_mp(
                        this, &HexMapIntNodeEditorPlugin::update_tiled_node));
        int_node->disconnect("editor_plugin_types_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::update_mesh_library));
        int_node = nullptr;

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

    if (tiled_node) {
        tiled_node->set_mesh_library(mesh_library);
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
    ERR_FAIL_COND_MSG(tiled_node == nullptr, "tiled node not allocated");
    tiled_node->set_cell(cell_id_v, type);
}

void HexMapIntNodeEditorPlugin::cell_scale_changed() {
    tiled_node->set_space(int_node->get_space());
    update_mesh_library();
}

void HexMapIntNodeEditorPlugin::set_cell_type(int id,
        String name,
        Color color) {
    ERR_FAIL_COND(int_node == nullptr);
    ERR_FAIL_COND(bottom_panel == nullptr);
    bool is_adding = id == HexMapIntNode::TypeIdNext;

    EditorUndoRedoManager *undo_redo = get_undo_redo();

    if (id == HexMapIntNode::TypeIdNext) {
        // This is a request to add a new type.
        //
        // work-around for dynamic assignment; if they didn't provide an id,
        // make the change directly now to get the id.  The do_method will just
        // repeat the same action with the fixed id we get here.
        //
        // We need a valid id here to be able to add the undo method.
        id = int_node->set_cell_type(id, name, color);
        undo_redo->create_action("HexMapInt: add cell type");
        undo_redo->add_undo_method(int_node, "remove_cell_type", id);
    } else {
        // To update an existing type, we need to get the current values for
        // undo
        const auto cell_type = int_node->cell_types.find(id);
        ERR_FAIL_COND_MSG(cell_type == int_node->cell_types.end(),
                "cell type id not found");
        undo_redo->create_action("HexMapInt: update cell type");
        undo_redo->add_undo_method(int_node,
                "set_cell_type",
                cell_type->key,
                cell_type->value.name,
                cell_type->value.color);
    }

    undo_redo->add_do_method(int_node, "set_cell_type", id, name, color);
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

void HexMapIntNodeEditorPlugin::set_tiled_map_visibility(bool visible) {
    ERR_FAIL_NULL(tiled_node);
    tiled_node->set_visible(visible);
}
#endif // TOOLS_ENABLED
