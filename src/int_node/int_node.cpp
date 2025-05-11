#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "core/cell_id.h"
#include "core/tile_orientation.h"
#include "int_node/int_node.h"

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
        type_id_max = 0;
        int count = types.size();
        for (int i = 0; i < count; i++) {
            const Dictionary &type = types[i];
            set_cell_type(type["value"], type["name"], type["color"]);
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

Variant HexMapIntNode::get_cell_type(unsigned id) const {
    Dictionary out;
    const auto cell_type = cell_types.find(id);
    ERR_FAIL_COND_V_MSG(
            cell_type == cell_types.end(), nullptr, "cell type id not found");
    out["value"] = id;
    out["name"] = cell_type->value.name;
    out["color"] = cell_type->value.color;
    return out;
}

void HexMapIntNode::remove_cell_type(unsigned id) {
    cell_types.erase(id);
    emit_signal("cell_types_changed");
}

unsigned HexMapIntNode::set_cell_type(unsigned id,
        const String name,
        const Color color) {
    if (id == TYPE_ID_NOT_SET) {
        id = type_id_max + 1;
    }
    if (id > type_id_max) {
        type_id_max = id;
    }

    struct CellType *entry = cell_types.getptr(id);
    if (entry != nullptr) {
        entry->name = name;
        entry->color = color;
    } else {
        cell_types.insert(id, CellType{ .name = name, .color = color });
    }

    emit_signal("cell_types_changed");
    return id;
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
    ClassDB::bind_method(
            D_METHOD("get_cell_type", "id"), &HexMapIntNode::get_cell_type);
    ClassDB::bind_method(D_METHOD("remove_cell_type", "id"),
            &HexMapIntNode::remove_cell_type);
    ClassDB::bind_method(D_METHOD("set_cell_type", "id", "name", "color"),
            &HexMapIntNode::set_cell_type);
    BIND_CONSTANT(TYPE_ID_NOT_SET);

    ADD_SIGNAL(MethodInfo("cell_types_changed"));
}

void HexMapIntNode::set_cell(const HexMapCellId &cell_id,
        int value,
        HexMapTileOrientation _) {
    if (value == HexMapNode::CELL_VALUE_NONE) {
        cell_map.erase(cell_id);
    } else if (value >= 0 && value < (1 << 16)) {
        cell_map.insert(cell_id, value);
    } else {
        ERR_FAIL_MSG("cell value must be in 0..65535");
    }
}

HexMapNode::CellInfo HexMapIntNode::get_cell(
        const HexMapCellId &cell_id) const {
    const uint16_t *current_cell = cell_map.getptr(cell_id);
    if (current_cell == nullptr) {
        return CellInfo{ .value = CELL_VALUE_NONE };
    }
    return CellInfo{ .value = *current_cell };
}

Array HexMapIntNode::get_cell_vecs() const {
    Array out;
    for (const auto &iter : cell_map) {
        out.push_back((Vector3i)iter.key);
    }
    return out;
}

Array HexMapIntNode::find_cell_vecs_by_value(int value) const {
    Array out;
    for (const auto &iter : cell_map) {
        if (iter.value == value) {
            out.push_back(static_cast<Vector3i>(iter.key));
        }
    }
    return out;
}
