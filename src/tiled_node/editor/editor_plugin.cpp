#ifdef TOOLS_ENABLED

#include <cassert>
#include <cstdint>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector3i.hpp>

#include "core/cell_id.h"
#include "core/editor/editor_cursor.h"
#include "core/editor/selection_manager.h"
#include "editor_control.h"
#include "editor_plugin.h"
#include "profiling.h"

using EditAxis = EditorCursor::EditAxis;

bool HexMapTiledNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapTiled");
}

// void HexMapTiledNodeEditorPlugin::_make_visible(bool p_visible) {
//     if (p_visible) {
//         editor_control->show();
//         set_process(true);
//     } else {
//         editor_control->hide();
//         set_process(false);
//     }
// }

void HexMapTiledNodeEditorPlugin::update_mesh_library() {
    ERR_FAIL_NULL(hex_map);

    mesh_library = tiled_node->get_mesh_library();
    bottom_panel->set("mesh_library", mesh_library);

    if (editor_cursor) {
        editor_cursor->clear_tiles();
        editor_cursor->set_mesh_library(mesh_library);
        bottom_panel->call("clear_selection");
        bottom_panel->call("update_mesh_list");
    }
}

void HexMapTiledNodeEditorPlugin::_edit(Object *p_object) {
    if (bottom_panel != nullptr) {
        remove_control_from_bottom_panel(bottom_panel);
        memfree(bottom_panel);
        bottom_panel = nullptr;
    }

    if (tiled_node) {
        // disconnect signals
        tiled_node->disconnect("hex_space_changed",
                callable_mp(this,
                        &HexMapTiledNodeEditorPlugin::
                                on_node_hex_space_changed));
        tiled_node->disconnect("mesh_offset_changed",
                callable_mp(this,
                        &HexMapTiledNodeEditorPlugin::
                                on_node_hex_space_changed));
        tiled_node->disconnect("mesh_library_changed",
                callable_mp(this,
                        &HexMapTiledNodeEditorPlugin::update_mesh_library));
        tiled_node = nullptr;

        // clear the mesh library
        mesh_library = Ref<MeshLibrary>();
        // bottom_panel->set("mesh_library", mesh_library);
    }

    HexMapNodeEditorPlugin::_edit(p_object);
    if (!hex_map) {
        return;
    }

    // get the node casted as a TiledNode
    tiled_node = Object::cast_to<HexMapTiledNode>(p_object);

    Ref<PackedScene> panel_scene = ResourceLoader::get_singleton()->load(
            "res://addons/hexmap/gui/"
            "hex_map_editor_bottom_panel.tscn");
    bottom_panel = (Control *)panel_scene->instantiate();
    bottom_panel->set("editor_plugin", this);
    add_control_to_bottom_panel(bottom_panel, "HexMap");
    make_bottom_panel_item_visible(bottom_panel);

    tiled_node->connect("hex_space_changed",
            callable_mp(this,
                    &HexMapTiledNodeEditorPlugin::on_node_hex_space_changed));
    tiled_node->connect("mesh_offset_changed",
            callable_mp(this,
                    &HexMapTiledNodeEditorPlugin::on_node_hex_space_changed));
    tiled_node->connect("mesh_library_changed",
            callable_mp(
                    this, &HexMapTiledNodeEditorPlugin::update_mesh_library));
    update_mesh_library();
}

void HexMapTiledNodeEditorPlugin::on_node_hex_space_changed() {
    ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
    editor_cursor->set_space(hex_map->get_space());
    selection_manager->set_space(hex_map->get_space());
}

void HexMapTiledNodeEditorPlugin::_bind_methods() {}

HexMapTiledNodeEditorPlugin::HexMapTiledNodeEditorPlugin() {
    // Ref<PackedScene> panel_scene = ResourceLoader::get_singleton()->load(
    //         "res://addons/hexmap/gui/"
    //         "hex_map_editor_bottom_panel.tscn");
    // bottom_panel = (Control *)panel_scene->instantiate();
}

HexMapTiledNodeEditorPlugin::~HexMapTiledNodeEditorPlugin() {}
#endif
