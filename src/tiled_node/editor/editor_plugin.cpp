#ifdef TOOLS_ENABLED

#include <cassert>
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

#include "core/editor/editor_cursor.h"
#include "editor_plugin.h"

using EditAxis = EditorCursor::EditAxis;

bool HexMapTiledNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapTiled");
}

HexMapTiledNodeEditorPlugin::HexMapTiledNodeEditorPlugin() {
    set_bottom_panel_scene(ResourceLoader::get_singleton()->load(
            "res://addons/hex_map/gui/"
            "hex_map_editor_bottom_panel.tscn"));
}

void HexMapTiledNodeEditorPlugin::on_tiled_node_mesh_library_changed() {
    ERR_FAIL_NULL(hex_map);

    if (editor_cursor) {
        editor_cursor->set_mesh_library(tiled_node->get_mesh_library());
        rebuild_cursor();
    }
}

void HexMapTiledNodeEditorPlugin::_edit(Object *p_object) {
    if (tiled_node) {
        // disconnect signals
        tiled_node->disconnect("mesh_origin_changed",
                callable_mp(this,
                        &HexMapTiledNodeEditorPlugin::
                                on_tiled_node_mesh_offset_changed));
        tiled_node->disconnect("mesh_library_changed",
                callable_mp(this,
                        &HexMapTiledNodeEditorPlugin::
                                on_tiled_node_mesh_library_changed));
        tiled_node = nullptr;

        // clear the mesh library
        mesh_library = Ref<MeshLibrary>();
    }

    HexMapNodeEditorPlugin::_edit(p_object);
    if (!hex_map) {
        return;
    }

    // get the node casted as a TiledNode
    tiled_node = Object::cast_to<HexMapTiledNode>(p_object);

    assert(bottom_panel != nullptr &&
            "bottom_panel should be set by superclass");
    bottom_panel->set("editor_plugin", this);
    bottom_panel->set("node", tiled_node);

    tiled_node->connect("mesh_origin_changed",
            callable_mp(this,
                    &HexMapTiledNodeEditorPlugin::
                            on_tiled_node_mesh_offset_changed));
    tiled_node->connect("mesh_library_changed",
            callable_mp(this,
                    &HexMapTiledNodeEditorPlugin::
                            on_tiled_node_mesh_library_changed));

    // update the editor cursor for the tiled_node state
    assert(editor_cursor != nullptr &&
            "editor_cursor should be set by superclass");
    editor_cursor->set_mesh_origin(tiled_node->get_mesh_origin_vec());
    on_tiled_node_mesh_library_changed();
}

void HexMapTiledNodeEditorPlugin::on_tiled_node_mesh_offset_changed() {
    ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
    editor_cursor->set_mesh_origin(tiled_node->get_mesh_origin_vec());
    editor_cursor->update();
}

#endif
