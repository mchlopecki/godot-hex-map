#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "editor_plugin.h"

bool HexMapIntNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapInt");
}

void HexMapIntNodeEditorPlugin::_edit(Object *p_object) {
    // called with null or object instance
    if (hex_map != nullptr) {
        remove_control_from_bottom_panel(bottom_panel);
        memfree(bottom_panel);

        set_process(false);
    }

    hex_map = Object::cast_to<HexMapIntNode>(p_object);
    if (hex_map == nullptr) {
        return;
    }

    // load the bottom panel scene and add it to the editor
    // XXX Move this loading to the constructor when gui stablizes
    Ref<PackedScene> panel_scene = ResourceLoader::get_singleton()->load(
            "res://addons/hexmap/gui/"
            "int_node_editor_bottom_panel.tscn");
    bottom_panel = (Control *)panel_scene->instantiate();
    bottom_panel->set("int_map", hex_map);
    add_control_to_bottom_panel(bottom_panel, "HexMapInt");
    make_bottom_panel_item_visible(bottom_panel);
}

#endif // TOOLS_ENABLED
