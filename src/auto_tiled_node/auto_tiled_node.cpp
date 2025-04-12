#include "auto_tiled_node.h"
#include "core/tile_orientation.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/variant.hpp"

using namespace godot;

Ref<MeshLibrary> HexMapAutoTiledNode::get_mesh_library() const {
    return mesh_library;
}

void HexMapAutoTiledNode::set_mesh_library(const Ref<MeshLibrary> &value) {
    mesh_library = value;
    tiled_node->set_mesh_library(value);
    emit_signal("mesh_library_changed");
}

Dictionary HexMapAutoTiledNode::get_rules() const {
    Dictionary out;
    for (const auto &r : rules) {
        Ref<HexMapTileRule> rule = r.value;
        out[r.key] = rule;
    }
    return out;
}

Array HexMapAutoTiledNode::get_rules_order() const {
    Array out;
    for (const auto &r : rules_order) {
        out.push_back(r);
    }
    return out;
}

unsigned HexMapAutoTiledNode::add_rule(const Rule &rule) {
    unsigned id = 0;
    for (int v : rules_order) {
        if (v >= id) {
            id = v + 1;
        }
    }

    auto iter = rules.insert(id, rule);
    iter->value.id = id;
    rules_order.push_back(id);

    apply_rules();
    emit_signal("rules_changed");
    return id;
}

unsigned HexMapAutoTiledNode::add_rule(const Ref<HexMapTileRule> &ref) {
    return add_rule(ref->inner);
}

void HexMapAutoTiledNode::update_rule(const Rule &rule) {
    Rule *entry = rules.getptr(rule.id);
    ERR_FAIL_NULL_MSG(entry, "update_rule() failed: rule id not found");
    *entry = rule;
    emit_signal("rules_changed");
}

void HexMapAutoTiledNode::update_rule(const Ref<HexMapTileRule> &ref) {
    return update_rule(ref->inner);
}

void HexMapAutoTiledNode::delete_rule(uint16_t id) { rules.erase(id); }

void HexMapAutoTiledNode::cell_scale_changed() {
    ERR_FAIL_NULL(int_node);
    ERR_FAIL_NULL(tiled_node);
    tiled_node->set_space(int_node->get_space());
}

void HexMapAutoTiledNode::apply_rules() {
    int max_range = 0;

    // get an iterator that gives me some ordered sample of the cell values
    // surrounding a given point.... so
    //      int_node->get_cell_neighbor_values(HexMapCellId)
    //
    for (const auto &iter : int_node->cell_map) {
        HexMapCellId cell_id = iter.key;
        int32_t cells[Rule::PatternCells];
        for (int i = 0; i < Rule::PatternCells; i++) {
            uint16_t *ptr =
                    int_node->cell_map.getptr(cell_id + Rule::CellOffsets[i]);
            cells[i] = ptr ? *ptr : -1;
        }

        for (int id : rules_order) {
            Rule &rule = rules[id];

            // do a quick exclude here for center tile
            for (int o = 0; o < 6; o++) {
                int matched = rule.match(cells, o);
                if (matched == -1) {
                    // pattern matched, set the tile and move on to the next
                    // cell.  Cells can only match one rule right now.
                    tiled_node->set_cell_item(
                            iter.key, rule.tile, static_cast<int>(o));
                    goto next_cell;
                } else if (matched == 0) {
                    // center most cell of pattern did not match, so there's no
                    // reason to try rotating the pattern to match.  Move on to
                    // next rule.
                    goto next_rule;
                }
            }
        next_rule:;
        }
    next_cell: /* noop */;
    }
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

    ClassDB::bind_method(
            D_METHOD("get_rules"), &HexMapAutoTiledNode::get_rules);
    ClassDB::bind_method(D_METHOD("get_rules_order"),
            &HexMapAutoTiledNode::get_rules_order);
    ClassDB::bind_method(D_METHOD("add_rule", "rule"),
            static_cast<unsigned (HexMapAutoTiledNode::*)(
                    const Ref<HexMapTileRule> &)>(
                    &HexMapAutoTiledNode::add_rule));
    ClassDB::bind_method(D_METHOD("update_rule", "rule"),
            static_cast<void (HexMapAutoTiledNode::*)(
                    const Ref<HexMapTileRule> &)>(
                    &HexMapAutoTiledNode::update_rule));
    ClassDB::bind_method(
            D_METHOD("delete_rule", "id"), &HexMapAutoTiledNode::delete_rule);

    ADD_SIGNAL(MethodInfo("rules_changed"));
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

void HexMapAutoTiledNode::Rule::clear_cell(HexMapCellId offset) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].set = false;
            return;
        }
    }
    ERR_FAIL_MSG("Rule::clear_cell(): invalid cell offset");
}

void HexMapAutoTiledNode::Rule::set_cell_type(HexMapCellId offset,
        unsigned type) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].set = true;
            pattern[i].type = type;
            return;
        }
    }
    ERR_FAIL_MSG("Rule::set_cell_type(): invalid cell offset");
}

// GDSCRIPT Rule BINDINGS
void HexMapAutoTiledNode::HexMapTileRule::_bind_methods() {
    ClassDB::bind_method(
            D_METHOD("set_tile", "tile"), &HexMapTileRule::set_tile);
    ClassDB::bind_method(D_METHOD("get_tile"), &HexMapTileRule::get_tile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tile", PROPERTY_HINT_NONE, ""),
            "set_tile",
            "get_tile");
    ClassDB::bind_method(D_METHOD("set_id", "tile"), &HexMapTileRule::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &HexMapTileRule::get_id);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "id", PROPERTY_HINT_NONE, ""),
            "set_id",
            "get_id");

    ClassDB::bind_method(
            D_METHOD("clear_cell", "offset"), &HexMapTileRule::clear_cell);
    ClassDB::bind_method(D_METHOD("set_cell_type", "offset", "type"),
            &HexMapTileRule::set_cell_type);
}

int HexMapAutoTiledNode::HexMapTileRule::get_tile() const {
    return inner.tile;
}
void HexMapAutoTiledNode::HexMapTileRule::set_tile(int value) {
    inner.tile = value;
}
unsigned HexMapAutoTiledNode::HexMapTileRule::get_id() const {
    return inner.id;
}
void HexMapAutoTiledNode::HexMapTileRule::set_id(unsigned value) {
    inner.id = value;
}
void HexMapAutoTiledNode::HexMapTileRule::clear_cell(
        const Ref<hex_bind::HexMapCellId> &offset) {
    inner.clear_cell(**offset);
}
void HexMapAutoTiledNode::HexMapTileRule::set_cell_type(
        const Ref<hex_bind::HexMapCellId> &offset,
        unsigned type) {
    inner.set_cell_type(**offset, type);
}
