#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "auto_tiled_node/editor/editor_plugin.h"

bool HexMapAutoTiledNodeEditorPlugin::_handles(Object *p_object) const {
    return p_object->is_class("HexMapAutoTiled");
}

void HexMapAutoTiledNodeEditorPlugin::_edit(Object *p_object) {
    if (auto_tiled_node) {
        // XXX disconnect things
        if (bottom_panel != nullptr) {
            remove_control_from_bottom_panel(bottom_panel);
            memfree(bottom_panel);
            bottom_panel = nullptr;
        }

        auto_tiled_node = nullptr;
    }

    auto_tiled_node = Object::cast_to<HexMapAutoTiledNode>(p_object);
    if (auto_tiled_node == nullptr) {
        return;
    }

    // XXX Move this loading to the constructor when gui stablizes
    // load the bottom panel scene and add it to the editor
    Ref<PackedScene> panel_scene = ResourceLoader::get_singleton()->load(
            "res://addons/hexmap/gui/"
            "auto_tiled_node_editor_bottom_panel.tscn");
    bottom_panel = (Control *)panel_scene->instantiate();
    bottom_panel->set("node", auto_tiled_node);
    bottom_panel->set("editor_plugin", this);
    // add & show the panel
    add_control_to_bottom_panel(bottom_panel, "HexMapAutoTiled");
    make_bottom_panel_item_visible(bottom_panel);
}

#endif // TOOLS_ENABLED
