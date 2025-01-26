#include "core/space.h"
#include "godot_cpp/classes/cylinder_mesh.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "tiled_node/tiled_node.h"
#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "editor_plugin.h"

bool HexMapIntNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapInt");
}

void HexMapIntNodeEditorPlugin::_edit(Object *p_object) {
    HexMapNodeEditorPlugin::_edit(p_object);

    if (hex_map) {
        int_map = Object::cast_to<HexMapIntNode>(p_object);
        tile_node = memnew(HexMapTiledNode);
        add_child(tile_node);
        update_mesh_library();

        // load the bottom panel scene and add it to the editor
        // XXX Move this loading to the constructor when gui stablizes
        Ref<PackedScene> panel_scene = ResourceLoader::get_singleton()->load(
                "res://addons/hexmap/gui/"
                "int_node_editor_bottom_panel.tscn");
        bottom_panel = (Control *)panel_scene->instantiate();
        bottom_panel->set("int_map", int_map);

        Node *type_selector = bottom_panel->get_node_or_null("%TypeSelector");
        if (type_selector) {
            type_selector->connect("types_changed",
                    callable_mp(this,
                            &HexMapIntNodeEditorPlugin::update_mesh_library));
        }
        add_control_to_bottom_panel(bottom_panel, "HexMapInt");
        make_bottom_panel_item_visible(bottom_panel);
    } else {
        int_map = nullptr;
        if (tile_node != nullptr) {
            remove_child(tile_node);
            memfree(tile_node);
            tile_node = nullptr;
        }
        // remove_child(tile_node);
        // memfree(tile_node);
        // tile_node = nullptr;

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
    const HexMapSpace &space = int_map->get_space();

    for (const auto &iter : int_map->get_cell_types()) {
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_albedo(iter.value.color);
        material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
        material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

        Ref<Mesh> mesh = space.build_cell_mesh();
        mesh->surface_set_material(0, material);

        mesh_library->create_item(iter.key);
        mesh_library->set_item_name(iter.key, iter.value.name);
        mesh_library->set_item_mesh(iter.key, mesh);
        if (tile_node) {
            tile_node->set_cell_item(
                    HexMapCellId(iter.key, iter.key, iter.key), iter.key);
        }
    }

    if (tile_node) {
        tile_node->set_mesh_library(mesh_library);
    }
}

#endif // TOOLS_ENABLED
