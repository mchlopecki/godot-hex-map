#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "core/space.h"
#include "editor_plugin.h"
#include "tiled_node/tiled_node.h"

bool HexMapIntNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapInt");
}

void HexMapIntNodeEditorPlugin::_edit(Object *p_object) {
    if (bottom_panel != nullptr) {
        remove_control_from_bottom_panel(bottom_panel);
        memfree(bottom_panel);
        bottom_panel = nullptr;
    }

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
        bottom_panel->connect("type_selected",
                callable_mp((HexMapNodeEditorPlugin *)this,
                        &HexMapNodeEditorPlugin::rebuild_cursor));
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
        bottom_panel->set("editor_plugin", this);
        bottom_panel->set("node", int_node);
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
        set_tiled_map_visibility(show_cells);
        update_mesh_library();

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
        int_node->connect("cells_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::
                                on_int_node_cells_changed));
        int_node->connect("cell_types_changed",
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
        int_node->disconnect("cells_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::
                                on_int_node_cells_changed));
        int_node->disconnect("cell_types_changed",
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
    // XXX this should be something provided by HexSpace, we use the math in
    // at least four places
    const Transform3D mesh_transform =
            Transform3D(Basis(), -space.get_mesh_offset());

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
        mesh_library->set_item_mesh_transform(iter.key, mesh_transform);
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

void HexMapIntNodeEditorPlugin::on_int_node_cells_changed(Array cells) {
    ERR_FAIL_COND_MSG(tiled_node == nullptr, "tiled node not allocated");
    tiled_node->set_cells(cells);
}

void HexMapIntNodeEditorPlugin::cell_scale_changed() {
    tiled_node->set_space(int_node->get_space());
    update_mesh_library();
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
