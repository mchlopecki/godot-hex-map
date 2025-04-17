#include "auto_tiled_node.h"
#include "core/iter_radial.h"
#include "core/tile_orientation.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"
#include <climits>

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
    assert(iter && "iter must return non-null");
    iter->value.id = id;
    iter->value.update_radius();
    rules_order.push_back(id);

    if (int_node) {
        apply_rules();
    }
    emit_signal("rules_changed");
    return id;
}

unsigned HexMapAutoTiledNode::add_rule(const Ref<HexMapTileRule> &ref) {
    UtilityFunctions::print("add_rule(): ", ref);
    return add_rule(ref->inner);
}

void HexMapAutoTiledNode::update_rule(const Rule &rule) {
    Rule *entry = rules.getptr(rule.id);
    ERR_FAIL_NULL_MSG(entry,
            "update_rule() failed: rule id not found: " + itos(rule.id));
    *entry = rule;
    entry->update_radius();
    if (int_node) {
        apply_rules();
    }
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

void HexMapAutoTiledNode::on_int_node_cells_changed(Array _cells) {
    if (int_node) {
        apply_rules();
    }
}

void HexMapAutoTiledNode::apply_rules() {
    ERR_FAIL_NULL(int_node);
    ERR_FAIL_NULL(tiled_node);

    tiled_node->clear();

    // cells go get based on the radius of the patterns
    int pattern_size = -1;

    // radius expanded by any rule that has an empty cell at pattern origin
    int border_radius = 0;
    for (const auto &iter : rules) {
        int size = iter.value.get_pattern_size();
        if (size > pattern_size) {
            pattern_size = size;
        }
        if (iter.value.pattern[0].state ==
                        HexMapAutoTiledNode::Rule::RULE_CELL_STATE_EMPTY &&
                iter.value.radius > border_radius) {
            border_radius = iter.value.radius;
        }
    }

    // if the number of cells to match in the pattern is less than one, nothing
    // to match here.
    if (pattern_size < 1) {
        return;
    }

    // XXX this is an unnecessary perf hit when border_radius == 0.
    HashSet<HexMapCellId::Key> cell_map;
    for (const auto &iter : int_node->cell_map) {
        cell_map.insert(iter.key);
        if (border_radius == 0) {
            continue;
        }

        HexMapCellId cell_id = iter.key;
        for (const HexMapCellId id :
                cell_id.get_neighbors(border_radius, HexMapPlanes::QRS)) {
            cell_map.insert(id);
        }
    }

    for (const auto key : cell_map) {
        HexMapCellId cell_id = key;
        int32_t cells[Rule::PatternCells];
        for (int i = 0; i < pattern_size; i++) {
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
                            cell_id, rule.tile, static_cast<int>(o));
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

HexMapNode::CellInfo HexMapAutoTiledNode::get_cell(
        const HexMapCellId &cell_id) const {
    return tiled_node->get_cell(cell_id);
}

Dictionary HexMapAutoTiledNode::_get_cell(
        const Ref<hex_bind::HexMapCellId> &ref) const {
    auto info = get_cell(ref->inner);
    Dictionary out;
    out["value"] = info.value;
    out["orientation"] = info.orientation;
    return out;
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

    ClassDB::bind_method(
            D_METHOD("get_cell", "cell_id"), &HexMapAutoTiledNode::_get_cell);
    ADD_SIGNAL(MethodInfo("rules_changed"));
}

void HexMapAutoTiledNode::_notification(int p_what) {
    static Transform3D last_transform;

    switch (p_what) {
    case NOTIFICATION_PARENTED:
        int_node = Object::cast_to<HexMapIntNode>(get_parent());
        ERR_FAIL_NULL_MSG(int_node,
                "Parent of HexMapAutoTiled node must be HexMapInt node");
        int_node->connect("cell_scale_changed",
                callable_mp(this, &HexMapAutoTiledNode::cell_scale_changed));
        int_node->connect("cells_changed",
                callable_mp(this,
                        &HexMapAutoTiledNode::on_int_node_cells_changed));
        cell_scale_changed();
        apply_rules();

        break;
    case NOTIFICATION_UNPARENTED:
        int_node->disconnect("cell_scale_changed",
                callable_mp(this, &HexMapAutoTiledNode::cell_scale_changed));
        int_node->disconnect("cells_changed",
                callable_mp(this,
                        &HexMapAutoTiledNode::on_int_node_cells_changed));
        int_node = nullptr;
        UtilityFunctions::print("before clear tiled ", tiled_node);
        tiled_node->clear();
        UtilityFunctions::print("clear tiled ", tiled_node);
        break;
    }
}

HexMapAutoTiledNode::HexMapAutoTiledNode() {
    int_node = nullptr;
    tiled_node = memnew(HexMapTiledNode);
    tiled_node->set_mesh_library(mesh_library);
    add_child(tiled_node);
    UtilityFunctions::print("tiled node = ", tiled_node);
}

HexMapAutoTiledNode::~HexMapAutoTiledNode() {
    UtilityFunctions::print(
            "remove child tiled ", vformat("0x%08x", (uint64_t)tiled_node));
    tiled_node = nullptr;
}

void HexMapAutoTiledNode::Rule::clear_cell(HexMapCellId offset) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].state = RULE_CELL_STATE_DISABLED;
            return;
        }
    }
    ERR_FAIL_MSG("Rule::clear_cell(): invalid cell offset");
}
HexMapAutoTiledNode::Rule::Cell HexMapAutoTiledNode::Rule::get_cell(
        HexMapCellId offset) const {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            return pattern[i];
        }
    }
    // XXX need a better way to denote an error here
    ERR_FAIL_V_MSG(Cell{ .type = USHRT_MAX },
            "Rule::get_cell(): invalid cell offset");
}

void HexMapAutoTiledNode::Rule::set_cell_type(HexMapCellId offset,
        unsigned type,
        bool invert) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].state =
                    invert ? RULE_CELL_STATE_NOT_TYPE : RULE_CELL_STATE_TYPE;
            pattern[i].type = type;
            return;
        }
    }
    ERR_FAIL_MSG("Rule::set_cell_type(): invalid cell offset");
}

void HexMapAutoTiledNode::Rule::set_cell_empty(HexMapCellId offset,
        bool invert) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].state =
                    invert ? RULE_CELL_STATE_NOT_EMPTY : RULE_CELL_STATE_EMPTY;
            return;
        }
    }
    ERR_FAIL_MSG("Rule::set_cell_empty(): invalid cell offset");
}

inline int HexMapAutoTiledNode::Rule::match(int32_t cell_type[PatternCells],
        HexMapTileOrientation orientation) {
    int o = static_cast<int>(orientation);
    assert(o >= 0 && o < 6 && "orientation must be in 0..5 inclusive");
    auto *pattern_index = PatternIndex[static_cast<int>(orientation)];

    int pattern_size = get_pattern_size();
    for (int i = 0; i < pattern_size; i++) {
        const auto &cell = pattern[pattern_index[i]];

        // match the cell state
        switch (cell.state) {
        case RULE_CELL_STATE_DISABLED:
            continue;
        case RULE_CELL_STATE_EMPTY:
            if (cell_type[i] == -1) {
                continue;
            }
            break;
        case RULE_CELL_STATE_NOT_EMPTY:
            if (cell_type[i] != -1) {
                continue;
            }
            break;
        case RULE_CELL_STATE_TYPE:
            if (cell_type[i] == pattern[pattern_index[i]].type) {
                continue;
            }
            break;
        case RULE_CELL_STATE_NOT_TYPE:
            if (cell_type[i] != pattern[pattern_index[i]].type) {
                continue;
            }
            break;
        }
        /// cell state did not match, return our index
        return i;
    }

    return -1;
}

void HexMapAutoTiledNode::Rule::update_radius() {
    int max_index = -1;
    for (int i = 0; i < PatternCells; i++) {
        if (pattern[i].state == RULE_CELL_STATE_DISABLED) {
            continue;
        }
        max_index = i;
        if (i > 6) {
            break;
        }
    }

    if (max_index > 6) {
        radius = 2;
    } else if (max_index > 0) {
        radius = 1;
    } else if (max_index == 0) {
        radius = 0;
    } else {
        ERR_FAIL_MSG("no cells set in pattern");
        radius = -1;
    }
}

inline unsigned HexMapAutoTiledNode::Rule::get_pattern_size() const {
    assert(radius <= 2 && "radius shoould not exceed 2");
    switch (radius) {
    case 0:
        return 1;
    case 1:
        return 7;
    case 2:
        return PatternCells;
    default:
        return 0;
    }
}

Dictionary HexMapAutoTiledNode::Rule::Cell::to_dict() const {
    Dictionary out;

    switch (state) {
    case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_EMPTY:
        out["state"] = "empty";
        out["type"] = nullptr;
        break;
    case Rule::RULE_CELL_STATE_NOT_EMPTY:
        out["state"] = "not_empty";
        out["type"] = nullptr;
        break;
    case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_DISABLED:
        out["state"] = "disabled";
        out["type"] = nullptr;
        break;
    case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_TYPE:
        out["state"] = "type";
        out["type"] = type;
        break;
    case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_NOT_TYPE:
        out["state"] = "not_type";
        out["type"] = type;
        break;
    }

    return out;
}

// GDSCRIPT Rule BINDINGS
void HexMapAutoTiledNode::HexMapTileRule::_bind_methods() {
    ClassDB::bind_method(
            D_METHOD("set_tile", "tile"), &HexMapTileRule::set_tile);
    ClassDB::bind_method(D_METHOD("get_tile"), &HexMapTileRule::get_tile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tile", PROPERTY_HINT_NONE, ""),
            "set_tile",
            "get_tile");

    ClassDB::bind_method(D_METHOD("set_id", "id"), &HexMapTileRule::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &HexMapTileRule::get_id);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "id", PROPERTY_HINT_NONE, ""),
            "set_id",
            "get_id");
    BIND_CONSTANT(RuleIdNotSet);

    ClassDB::bind_method(
            D_METHOD("get_cell", "offset"), &HexMapTileRule::get_cell);
    ClassDB::bind_method(D_METHOD("get_cells"), &HexMapTileRule::get_cells);
    ClassDB::bind_method(
            D_METHOD("clear_cell", "offset"), &HexMapTileRule::clear_cell);
    ClassDB::bind_method(D_METHOD("set_cell_type", "offset", "type", "invert"),
            &HexMapTileRule::set_cell_type,
            DEFVAL(false));
    ClassDB::bind_method(D_METHOD("set_cell_empty", "offset", "invert"),
            &HexMapTileRule::set_cell_empty,
            DEFVAL(false));
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
        unsigned type,
        bool invert) {
    inner.set_cell_type(**offset, type, invert);
}
void HexMapAutoTiledNode::HexMapTileRule::set_cell_empty(
        const Ref<hex_bind::HexMapCellId> &ref,
        bool invert) {
    inner.set_cell_empty(ref->inner, invert);
}

Dictionary HexMapAutoTiledNode::HexMapTileRule::get_cell(
        const Ref<hex_bind::HexMapCellId> &ref) const {
    Dictionary out;
    auto cell = inner.get_cell(ref->inner);
    return cell.to_dict();
}

Array HexMapAutoTiledNode::HexMapTileRule::get_cells() const {
    Array out;
    out.resize(Rule::PatternCells);
    for (int i = 0; i < Rule::PatternCells; i++) {
        Dictionary dict = inner.pattern[i].to_dict();
        dict["offset"] = Rule::CellOffsets[i].to_ref();
        out[i] = dict;
    }
    return out;
}

String HexMapAutoTiledNode::HexMapTileRule::_to_string() const {
    // clang-format off
    String fmt = "Rule {id}: tile {tile}\n"
        "        v---^---v---^---v---^---v\n"
        "        | {15} | {14} | {13} |\n"
        "    v---^---v---^---v---^---v---^---v\n"
        "    | {16} | {5} | {4} | {12} |\n"
        "v---^---v---^---v---^---v---^---v---^---v\n"
        "| {17} | {6} | {0} | {3} | {11} |\n"
        "^---v---^---v---^---v---^---v---^---v---^\n"
        "    | {18} | {1} | {2} | {10} |\n"
        "    ^---v---^---v---^---v---^---v---^\n"
        "        | {7} | {8} | {9} |\n"
        "        ^---v---^---v---^---v---^\n";
    // clang-format on

    Dictionary fields;
    fields["id"] = inner.id;
    fields["tile"] = inner.tile;

    for (int i = 0; i < Rule::PatternCells; i++) {
        auto cell = inner.pattern[i];
        switch (cell.state) {
        case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_DISABLED:
            fields[itos(i)] = "     ";
            break;
        case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_EMPTY:
            fields[itos(i)] = "  !  ";
            break;
        case Rule::RULE_CELL_STATE_NOT_EMPTY:
            fields[itos(i)] = "  +  ";
            break;
        case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_TYPE:
            fields[itos(i)] = vformat("%-5d", inner.pattern[i].type);
            break;
        case HexMapAutoTiledNode::Rule::RULE_CELL_STATE_NOT_TYPE:
            fields[itos(i)] = vformat("! %-3d", inner.pattern[i].type);
            break;
        }
    }

    return fmt.format(fields);
}
