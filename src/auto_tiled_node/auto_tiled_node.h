#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include "core/cell_id.h"
#include "core/hex_map_node.h"
#include "godot_cpp/classes/mesh_library.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "int_node/int_node.h"
#include "tiled_node/tiled_node.h"

using namespace godot;

class HexMapAutoTiledNodeEditorPlugin;

class HexMapAutoTiledNode : public Node3D {
    using HexMapAutoTiled = HexMapAutoTiledNode;
    GDCLASS(HexMapAutoTiled, Node3D);

    friend HexMapAutoTiledNodeEditorPlugin;

public:
    HexMapAutoTiledNode();
    ~HexMapAutoTiledNode();

    // HexMapNode::CellInfo get_cell(const HexMapCellId &) const;
    // Array get_cell_ids_v() const;
    // void set_cell_visibility(const HexMapCellId &cell_id, bool visibility);

    // property setters & getters
    void set_mesh_library(const Ref<MeshLibrary> &);
    Ref<MeshLibrary> get_mesh_library() const { return mesh_library; }

    // signal callbacks
    void cell_scale_changed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

    // save/load support
    // void _get_property_list(List<PropertyInfo> *p_list) const;
    // bool _get(const StringName &p_name, Variant &r_ret) const;
    // bool _set(const StringName &p_name, const Variant &p_value);

private:
    Ref<MeshLibrary> mesh_library;
    HexMapTiledNode *tiled_node;
    HexMapIntNode *int_node;
};
