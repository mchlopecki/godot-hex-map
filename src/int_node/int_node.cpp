#include "int_node/int_node.h"
#include "core/cell_id.h"
#include "core/tile_orientation.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"
#include <climits>

void HexMapIntNode::_get_property_list(List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::ARRAY,
            "cell_types",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
    p_list->push_back(PropertyInfo(Variant::PACKED_BYTE_ARRAY,
            "cells",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
}

bool HexMapIntNode::_get(const StringName &p_name, Variant &r_ret) const {
    String name = p_name;

    if (name == "cell_types") {
        r_ret = _get_cell_types();
        return true;
    } else if (name == "cells") {
        PackedByteArray cells;
        cells.resize(cell_map.size() * 8);
        size_t offset = 0;
        for (const auto &iter : cell_map) {
            cells.encode_s16(offset, iter.key.q);
            offset += 2;
            cells.encode_s16(offset, iter.key.r);
            offset += 2;
            cells.encode_s16(offset, iter.key.y);
            offset += 2;
            cells.encode_u16(offset, iter.value);
            offset += 2;
        }
        r_ret = cells;
        return true;
    }

    return false;
}
bool HexMapIntNode::_set(const StringName &p_name, const Variant &p_value) {
    String name = p_name;

    if (name == "cell_types") {
        const Array types = p_value;
        cell_types.clear();
        max_type = 0;
        int count = types.size();
        for (int i = 0; i < count; i++) {
            const Dictionary &type = types[i];
            unsigned value = type["value"];
            cell_types[value] = CellType{
                .name = type["name"],
                .color = type["color"],
            };
            if (value >= max_type) {
                max_type = value + 1;
            }
        }
        return true;
    } else if (name == "cells") {
        const PackedByteArray cells = p_value;
        ERR_FAIL_COND_V_MSG(cells.size() % 8 != 0,
                false,
                "HexMapIntNode cells PackedByteArray must be a multiple of 8");
        size_t offset = 0, max = cells.size();
        while (offset < max) {
            HexMapCellId::Key key;
            key.q = cells.decode_s16(offset);
            offset += 2;
            key.r = cells.decode_s16(offset);
            offset += 2;
            key.y = cells.decode_s16(offset);
            offset += 2;

            unsigned value = cells.decode_u16(offset);
            offset += 2;

            cell_map[key] = value;
        }
        return true;
    }
    return false;
}

unsigned HexMapIntNode::_add_cell_type(const String name, const Color color) {
    unsigned id = max_type++;
    cell_types.insert(id, CellType{ .name = name, .color = color });
    return id;
}

void HexMapIntNode::remove_cell_type(unsigned id) { cell_types.erase(id); }

bool HexMapIntNode::update_cell_type(unsigned id,
        const String name,
        const Color color) {
    struct CellType *entry = cell_types.getptr(id);
    ERR_FAIL_NULL_V_MSG(
            entry, false, "HexMapIntNode: unknown cell type " + itos(id));

    entry->name = name;
    entry->color = color;
    return true;
}

Array HexMapIntNode::_get_cell_types() const {
    Array out;
    for (const auto &it : cell_types) {
        Dictionary type;
        type["value"] = it.key;
        type["name"] = it.value.name;
        type["color"] = it.value.color;
        out.push_back(type);
    }
    return out;
}

void HexMapIntNode::_bind_methods() {
    ClassDB::bind_method(
            D_METHOD("get_cell_types"), &HexMapIntNode::_get_cell_types);
    ClassDB::bind_method(D_METHOD("add_cell_type", "name", "color"),
            &HexMapIntNode::_add_cell_type);
    ClassDB::bind_method(D_METHOD("remove_cell_type", "id"),
            &HexMapIntNode::remove_cell_type);
    ClassDB::bind_method(
            D_METHOD("update_cell_type", "value", "name", "color"),
            &HexMapIntNode::update_cell_type);
#ifdef TOOLS_ENABLED
    ADD_SIGNAL(MethodInfo("editor_plugin_cell_changed",
            PropertyInfo(Variant::VECTOR3I, "cell"),
            PropertyInfo(Variant::INT, "type")));
#endif // TOOLS_ENABLED
}

void HexMapIntNode::set_cell(const HexMapCellId &cell_id,
        int value,
        HexMapTileOrientation _) {
    if (value == HexMapNode::INVALID_CELL_ITEM) {
        cell_map.erase(cell_id);
    } else if (value >= 0 && value < (1 << 16)) {
        cell_map.insert(cell_id, value);
    } else {
        ERR_FAIL_MSG("cell value must be in 0..65535");
    }
#ifdef TOOLS_ENABLED
    emit_signal("editor_plugin_cell_changed", (Vector3i)cell_id, value);
#endif // TOOLS_ENABLED
}

HexMapNode::CellInfo HexMapIntNode::get_cell(
        const HexMapCellId &cell_id) const {
    const uint16_t *current_cell = cell_map.getptr(cell_id);
    if (current_cell == nullptr) {
        return CellInfo{ .value = INVALID_CELL_ITEM };
    }
    return CellInfo{ .value = *current_cell };
}

Array HexMapIntNode::get_cell_ids_v() const {
    Array out;
    for (const auto &iter : cell_map) {
        out.push_back((Vector3i)iter.key);
    }
    return out;
}
