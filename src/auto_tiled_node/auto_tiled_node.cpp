#include "auto_tiled_node.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/variant.hpp"

using namespace godot;

void HexMapAutoTiledNode::set_mesh_library(const Ref<MeshLibrary> &value) {
    mesh_library = value;
    tiled_node->set_mesh_library(value);
    emit_signal("mesh_library_changed");
}

void HexMapAutoTiledNode::cell_scale_changed() {
    ERR_FAIL_NULL(int_node);
    ERR_FAIL_NULL(tiled_node);
    tiled_node->set_space(int_node->get_space());
}

void HexMapAutoTiledNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_mesh_library", "mesh_library"),
            &HexMapAutoTiledNode::set_mesh_library);
    ClassDB::bind_method(D_METHOD("get_mesh_library"),
            &HexMapAutoTiledNode::get_mesh_library);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
                         "mesh_library",
                         PROPERTY_HINT_RESOURCE_TYPE,
                         "MeshLibrary"),
            "set_mesh_library",
            "get_mesh_library");
    ADD_SIGNAL(MethodInfo("mesh_library_changed"));
}

void HexMapAutoTiledNode::_notification(int p_what) {
    static Transform3D last_transform;

    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            int_node = Object::cast_to<HexMapIntNode>(get_parent());
            ERR_FAIL_NULL_MSG(int_node,
                    "Parent of HexMapAutoTiled node must be HexMapInt node");
            int_node->connect("cell_scale_changed",
                    callable_mp(
                            this, &HexMapAutoTiledNode::cell_scale_changed));
            tiled_node->set_space(int_node->get_space());
            tiled_node->set_mesh_library(mesh_library);
            tiled_node->set_cell_item(HexMapCellId(), 0);
            add_child(tiled_node);
            break;
        }
        case NOTIFICATION_EXIT_TREE: {
            int_node->disconnect("cell_scale_changed",
                    callable_mp(
                            this, &HexMapAutoTiledNode::cell_scale_changed));
            int_node = nullptr;
            remove_child(tiled_node);
            break;
        }
    }
}

HexMapAutoTiledNode::HexMapAutoTiledNode() {
    tiled_node = memnew(HexMapTiledNode);
    int_node = nullptr;
}

HexMapAutoTiledNode::~HexMapAutoTiledNode() { memfree(tiled_node); }
