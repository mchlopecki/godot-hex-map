#ifdef TOOLS_ENABLED

#pragma once

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/slider.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "../tiled_node.h"
#include "core/editor/editor_cursor.h"
#include "core/editor/editor_plugin.h"
#include "core/editor/selection_manager.h"
#include "editor_control.h"

using namespace godot;

class HexMapTiledNodeEditorPlugin : public HexMapNodeEditorPlugin {
    GDCLASS(HexMapTiledNodeEditorPlugin, HexMapNodeEditorPlugin);

public:
    // virtual void _make_visible(bool visible) override;
    virtual bool _handles(Object *object) const override;
    virtual void _edit(Object *p_object) override;

    HexMapTiledNodeEditorPlugin();
    ~HexMapTiledNodeEditorPlugin();

protected:
    static void _bind_methods();

private:
    HexMapTiledNode *tiled_node = nullptr;
    Ref<MeshLibrary> mesh_library = nullptr;

    // callbacks for HexMap signals
    void update_mesh_library();
    void on_node_hex_space_changed();

    // callbacks used by signals from EditorControl
    void tile_changed(int p_mesh_id);
    void plane_changed(int p_axis);
    void axis_changed(int p_axis);
    void cursor_changed(Variant p_orientation);
};

#endif
