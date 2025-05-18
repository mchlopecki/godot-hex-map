#include <cassert>
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

HexMapIntNodeEditorPlugin::HexMapIntNodeEditorPlugin() {
    set_bottom_panel_scene(ResourceLoader::get_singleton()->load(
            "res://addons/hex_map/gui/"
            "int_node_editor_bottom_panel.tscn"));
}

void HexMapIntNodeEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_cells_visible", "axis"),
            &HexMapIntNodeEditorPlugin::set_cells_visible);
    ClassDB::bind_method(D_METHOD("get_cells_visible"),
            &HexMapIntNodeEditorPlugin::get_cells_visible);
    ADD_PROPERTY(PropertyInfo(Variant::INT,
                         "cells_visible",
                         godot::PROPERTY_HINT_NONE,
                         ""),
            "set_cells_visible",
            "get_cells_visible");
}

bool HexMapIntNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapInt");
}

void HexMapIntNodeEditorPlugin::_edit(Object *p_object) {
    if (int_node) {
        // save off the show_cells state
        int_node->set_meta("_show_cells_", tiled_node->is_visible());

        // free the tiled node
        int_node->remove_child(tiled_node);
        tiled_node->queue_free();
        tiled_node = nullptr;

        // disconnect all of our signals
        int_node->disconnect("hex_space_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::
                                on_int_node_hex_space_changed));
        int_node->disconnect("cells_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::
                                on_int_node_cells_changed));
        int_node->disconnect("cell_types_changed",
                callable_mp(this,
                        &HexMapIntNodeEditorPlugin::
                                on_int_node_cell_types_changed));
        int_node = nullptr;
    }

    HexMapNodeEditorPlugin::_edit(p_object);
    if (hex_map == nullptr) {
        return;
    }

    // get IntNode properly casted and connect those signals we care about
    int_node = Object::cast_to<HexMapIntNode>(p_object);
    int_node->connect("hex_space_changed",
            callable_mp(this,
                    &HexMapIntNodeEditorPlugin::
                            on_int_node_hex_space_changed));
    int_node->connect("cells_changed",
            callable_mp(this,
                    &HexMapIntNodeEditorPlugin::on_int_node_cells_changed));
    int_node->connect("cell_types_changed",
            callable_mp(this,
                    &HexMapIntNodeEditorPlugin::
                            on_int_node_cell_types_changed));

    // create a TiledNode to visualize the contents of the IntNode
    tiled_node = memnew(HexMapTiledNode);
    tiled_node->set_space(int_node->get_space());

    // call the cell_types changed callback to build the visualization
    // MeshLibrary, and set it in the tiled_node.
    on_int_node_cell_types_changed();

    // populate the tiled node with our cells
    for (const auto &iter : int_node->cell_map) {
        tiled_node->set_cell(iter.key, iter.value);
    }

    // add the tiled node to the int_node to inherit the visibility of the
    // int node.
    int_node->add_child(tiled_node);

    // set known state
    assert(bottom_panel != nullptr);
    bottom_panel->set("editor_plugin", this);
    bottom_panel->set("node", int_node);
    bottom_panel->set("show_cells", int_node->get_meta("_show_cells_", true));
}

void HexMapIntNodeEditorPlugin::on_int_node_cell_types_changed() {
    mesh_library.instantiate();
    const HexMapSpace &space = int_node->get_space();

    for (const auto &iter : int_node->get_cell_types()) {
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_albedo(iter.value.color);
        if (iter.value.color.a < 1.0) {
            material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
        }
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
}

void HexMapIntNodeEditorPlugin::on_int_node_cells_changed(Array cells) {
    ERR_FAIL_COND_MSG(tiled_node == nullptr, "tiled node not allocated");
    tiled_node->set_cells(cells);
}

void HexMapIntNodeEditorPlugin::on_int_node_hex_space_changed() {
    tiled_node->set_space(int_node->get_space());
    on_int_node_cell_types_changed();
}

void HexMapIntNodeEditorPlugin::set_cells_visible(bool visible) {
    ERR_FAIL_NULL(tiled_node);
    tiled_node->set_visible(visible);
}

bool HexMapIntNodeEditorPlugin::get_cells_visible() const {
    return tiled_node ? tiled_node->is_visible() : false;
}

#endif // TOOLS_ENABLED
