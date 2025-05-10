#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "auto_tiled_node.h"
#include "core/iter_radial.h"
#include "core/tile_orientation.h"

void HexMapAutoTiledNode::_get_property_list(
        List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::ARRAY,
            "rules_order",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
    p_list->push_back(PropertyInfo(Variant::DICTIONARY,
            "rules",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
}

bool HexMapAutoTiledNode::_get(const StringName &p_name,
        Variant &r_ret) const {
    String name = p_name;

    if (name == "rules_order") {
        // saving the rules order is straight forward
        r_ret = get_rules_order();
        return true;

    } else if (name == "rules") {
        // saving rules is more complicated, create a dictionary of the rule id
        // to a dictionary defining the rule.
        Dictionary out;
        for (const auto &iter : rules) {
            Dictionary value;
            value["id"] = iter.value.id;
            value["tile"] = iter.value.tile;
            value["enabled"] = iter.value.enabled;

            // create a cells dictionary, mapping cell offset (as Vector3i) to
            // a dictionary for the cell value.
            Dictionary cells;
            for (int i = 0; i < Rule::PatternCells; i++) {
                const Rule::Cell &cell = iter.value.pattern[i];
                if (cell.state == Rule::RULE_CELL_STATE_DISABLED) {
                    continue;
                }

                Vector3i key = Rule::CellOffsets[i];
                cells[key] = cell.to_dict();
            }
            value["cells"] = cells;

            out[iter.key] = value;
        }
        r_ret = out;
        return true;
    }

    return false;
}
bool HexMapAutoTiledNode::_set(const StringName &p_name,
        const Variant &p_value) {
    String name = p_name;

    if (name == "rules_order") {
        // manually copy over the rules order
        const Array value = p_value;
        int count = value.size();
        rules_order.clear();
        rules_order.resize(count);
        for (int i = 0; i < count; i++) {
            rules_order.set(i, value[i]);
        }

        return true;
    } else if (name == "rules") {
        // iterate through the rules dictionary
        Dictionary value = p_value;
        Array keys = value.keys();
        size_t count = keys.size();
        for (size_t i = 0; i < count; i++) {
            int id = keys[i];
            Dictionary rule_dict = value[id];

            // create a new rule fron the rule dictionary
            Rule rule{};
            rule.id = id;
            rule.tile = rule_dict["tile"];
            rule.enabled = rule_dict["enabled"];

            // iterate through the cells dictionary within the rule dictionary
            Dictionary cells_dict = rule_dict["cells"];
            Array cells_keys = cells_dict.keys();
            size_t cell_count = cells_keys.size();
            for (size_t c = 0; c < cell_count; c++) {
                Vector3i offset = cells_keys[c];

                // convert the cell dictionary into a Cell instance
                Rule::Cell cell = Rule::Cell::from_dict(cells_dict[offset]);

                // look up the pattern index for the cell offset
                int index = rule.get_pattern_index(offset);
                if (index == -1) {
                    ERR_PRINT("invalid HexMapAutoTiledNode::Rule cell offset");
                    continue;
                }

                // set the cell into the pattern
                rule.pattern[index] = cell;
            }

            // now that we have a complete rule, update its internal state,
            // then add it to the rules hash.
            rule.update_internal();
            rules.insert(id, rule);
        }

        // apply the rules we just loaded.
        return true;
    }
    return false;
}

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

void HexMapAutoTiledNode::set_rules_order(Array value) {
    rules_order.clear();
    size_t size = value.size();
    rules_order.resize(size);
    for (size_t i = 0; i < size; i++) {
        rules_order.set(i, value[i]);
    }
    if (int_node) {
        apply_rules();
    }
    emit_signal("rules_changed");
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
    rules_order.push_back(id);

    if (int_node) {
        apply_rules();
    }
    emit_signal("rules_changed");
    return id;
}

unsigned HexMapAutoTiledNode::add_rule(const Ref<HexMapTileRule> &ref) {
    return add_rule(ref->inner);
}

void HexMapAutoTiledNode::update_rule(const Rule &rule) {
    Rule *entry = rules.getptr(rule.id);
    ERR_FAIL_NULL_MSG(entry,
            "update_rule() failed: rule id not found: " + itos(rule.id));
    *entry = rule;
    if (int_node) {
        apply_rules();
    }
    emit_signal("rules_changed");
}

void HexMapAutoTiledNode::update_rule(const Ref<HexMapTileRule> &ref) {
    return update_rule(ref->inner);
}

void HexMapAutoTiledNode::delete_rule(uint16_t id) {
    rules.erase(id);
    rules_order.erase(id);
    if (int_node) {
        apply_rules();
    }
    emit_signal("rules_changed");
}

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

    // union the cell masks from all rules to determine which neighboring cells
    // we neet to fetch to match rules
    uint64_t cell_mask = 0;

    // do something similar for the cell padding.  Calculate the padding we
    // need for all cells across y=-2...2 to allow for matching against rules
    // with an empty cell at the center of the pattern.  We normally only
    // apply the rules to those cells that have some value defined, but to
    // support a pattern matching an empty cell (no value assigned) at the
    // center of the pattern, we need to evaluate cells with no value set.  To
    // prevent evaluating every command, we only expand our search area around
    // existing cells based on the rules defined.
    //
    // As an example, a rule with the center cell empty, but the cell below it
    // must have the value 3, we'll expand the cell ids to include the cell id
    // **above** every cell that is defined.
    int8_t cell_padding[5] = { -1, -1, -1, -1, -1 };
    for (const auto &iter : rules) {
        cell_mask |= iter.value.cell_mask;

        // manually merge the cell pad (terrible name; fix later)
        auto pad = iter.value.cell_pad;
        int index = 2 + pad.delta_y;
        if (pad.radius > cell_padding[index]) {
            cell_padding[index] = pad.radius;
        }
    }

    // expand the search space for cell padding needed to support empty tile
    // rules.  This is a small penalty hit when there are no empty-center-cell
    // rules defined.
    HashSet<HexMapCellId::Key> cell_map;
    for (const auto &iter : int_node->cell_map) {
        cell_map.insert(iter.key);

        HexMapCellId cell_id = iter.key;
        // UtilityFunctions::print("insert cell " + cell_id);

        for (int i = 0; i < 5; i++) {
            if (cell_padding[i] < 0) {
                continue;
            }
            int radius = cell_padding[i];
            HexMapCellId cell_id = iter.key;
            cell_id.y += -2 + i;

            for (const HexMapCellId id :
                    cell_id.get_neighbors(radius, HexMapPlanes::QRS, true)) {
                // UtilityFunctions::print("insert pad cell " + id);
                cell_map.insert(id);
            }
        }
    }

    for (const auto key : cell_map) {
        HexMapCellId cell_id = key;
        int32_t cells[Rule::PatternCells];
        for (int i = 0; i < Rule::PatternCells; i++) {
            if ((cell_mask & (1ULL << i)) == 0) {
                cells[i] = -1;
                continue;
            }
            uint16_t *ptr =
                    int_node->cell_map.getptr(cell_id + Rule::CellOffsets[i]);
            cells[i] = ptr ? *ptr : -1;
        }
        for (int id : rules_order) {
            Rule &rule = rules[id];

            if (!rule.enabled) {
                continue;
            }

            for (int o = 0; o < 6; o++) {
                int matched = rule.match(cells, o);
                if (matched == -1) {
                    // pattern matches with the current orientation, set the
                    // tile & orientation in the tiled node, and move on to the
                    // next cell for evaluation.
                    tiled_node->set_cell_item(
                            cell_id, rule.tile, static_cast<int>(o));
                    goto next_cell;
                } else if (matched < 5) {
                    // XXX fix ugly magic number
                    //
                    // If one of the center cells (q = r = 0, y = -2...2) does
                    // not match, no ammount of rotation will make it match, so
                    // just move on to the next rule.
                    goto next_rule;
                }
            }
        next_rule: /* noop */;
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
    ClassDB::bind_method(D_METHOD("set_rules_order", "order"),
            &HexMapAutoTiledNode::set_rules_order);
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

int HexMapAutoTiledNode::Rule::get_pattern_index(HexMapCellId offset) const {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            return i;
        }
    }
    return -1;
}

void HexMapAutoTiledNode::Rule::clear_cell(HexMapCellId offset) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].state = RULE_CELL_STATE_DISABLED;
            update_internal();
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
    // don't print an error here because this is used to check for a valid
    // cell offset in the editor
    return Cell{ .state = RULE_CELL_INVALID_OFFSET };
}

void HexMapAutoTiledNode::Rule::set_cell_type(HexMapCellId offset,
        unsigned type,
        bool invert) {
    for (int i = 0; i < PatternCells; i++) {
        if (offset == CellOffsets[i]) {
            pattern[i].state =
                    invert ? RULE_CELL_STATE_NOT_TYPE : RULE_CELL_STATE_TYPE;
            pattern[i].type = type;
            update_internal();
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
            update_internal();
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

    int pattern_size = PatternCells;
    for (int i = 0; i < pattern_size; i++) {
        int index = pattern_index[i];
        const auto &cell = pattern[pattern_index[i]];

        // match the cell state
        switch (cell.state) {
        case RULE_CELL_STATE_DISABLED:
        case RULE_CELL_INVALID_OFFSET:
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
            if (cell_type[i] == cell.type) {
                continue;
            }
            break;
        case RULE_CELL_STATE_NOT_TYPE:
            if (cell_type[i] != cell.type) {
                continue;
            }
            break;
        }
        /// cell state did not match, return our index
        return i;
    }

    return -1;
}

void HexMapAutoTiledNode::Rule::update_internal() {
    cell_mask = 0;
    for (int i = 0; i < PatternCells; i++) {
        if (pattern[i].state == RULE_CELL_STATE_DISABLED) {
            continue;
        }

        if (i < 5) {
            cell_mask |= (1 << i);
        } else if (i < 11) {
            cell_mask |= 0b111111 << 5;
            i = 10;
        } else if (i < 17) {
            cell_mask |= 0b111111 << 11;
            i = 16;
        } else if (i < 23) {
            cell_mask |= 0b111111 << 17;
            i = 22;
        } else {
            cell_mask |= (uint64_t)0xfff << 23;
            break;
        }
    }

    // calculate any cell padding that may be needed for this rule
    cell_pad = { .delta_y = 0, .radius = -1 };
    if (pattern[0].state != RULE_CELL_STATE_EMPTY) {
        return;
    }
    for (int i = 0; i < PatternCells; i++) {
        if (pattern[i].state == RULE_CELL_STATE_DISABLED ||
                pattern[i].state == RULE_CELL_STATE_EMPTY) {
            continue;
        }

        if (i == 0) {
            continue;
        } else if (i == 1) {
            cell_pad = { .delta_y = -1, .radius = 0 };
            break;
        } else if (i == 2) {
            cell_pad = { .delta_y = 1, .radius = 0 };
            break;
        } else if (i == 3) {
            cell_pad = { .delta_y = -2, .radius = 0 };
            break;
        } else if (i == 4) {
            cell_pad = { .delta_y = 2, .radius = 0 };
            break;
        } else if (i < 11) {
            cell_pad = { .delta_y = 0, .radius = 1 };
            break;
        } else if (i < 17) {
            cell_pad = { .delta_y = -1, .radius = 1 };
            break;
        } else if (i < 23) {
            cell_pad = { .delta_y = 1, .radius = 1 };
            break;
        } else {
            cell_pad = { .delta_y = 0, .radius = 2 };
            break;
        }
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
    case RULE_CELL_INVALID_OFFSET:
        assert(false && "cannot create dictionary for invalid offset");
        break;
    }

    return out;
}

HexMapAutoTiledNode::Rule::Cell HexMapAutoTiledNode::Rule::Cell::from_dict(
        const Dictionary value) {
    Cell out{};

    if (value["state"] == "empty") {
        out.state = RULE_CELL_STATE_EMPTY;
    } else if (value["state"] == "not_empty") {
        out.state = RULE_CELL_STATE_NOT_EMPTY;
    } else if (value["state"] == "disabled") {
        out.state = RULE_CELL_STATE_DISABLED;
    } else if (value["state"] == "type") {
        out.state = RULE_CELL_STATE_TYPE;
        out.type = value["type"];
    } else if (value["state"] == "not_type") {
        out.state = RULE_CELL_STATE_NOT_TYPE;
        out.type = value["type"];
    } else {
        ERR_PRINT("invalid state in rule dictionary");
        out.state = RULE_CELL_INVALID_OFFSET;
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
            D_METHOD("set_enabled", "enabled"), &HexMapTileRule::set_enabled);
    ClassDB::bind_method(
            D_METHOD("get_enabled"), &HexMapTileRule::get_enabled);
    ADD_PROPERTY(
            PropertyInfo(Variant::BOOL, "enabled", PROPERTY_HINT_NONE, ""),
            "set_enabled",
            "get_enabled");

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
bool HexMapAutoTiledNode::HexMapTileRule::get_enabled() const {
    return inner.enabled;
}
void HexMapAutoTiledNode::HexMapTileRule::set_enabled(bool value) {
    inner.enabled = value;
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

Variant HexMapAutoTiledNode::HexMapTileRule::get_cell(
        const Ref<hex_bind::HexMapCellId> &ref) const {
    Dictionary out;
    auto cell = inner.get_cell(ref->inner);
    if (cell.state == Rule::RULE_CELL_INVALID_OFFSET) {
        return nullptr;
    } else {
        return cell.to_dict();
    }
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
    // XXX expand to print y=-2...2
    // clang-format off
    String fmt = "Rule {id}: tile {tile}\n"
        "        v---^---v---^---v---^---v\n"
        "        | {31} | {30} | {29} |\n"
        "    v---^---v---^---v---^---v---^---v\n"
        "    | {32} | {9} | {8} | {28} |\n"
        "v---^---v---^---v---^---v---^---v---^---v\n"
        "| {33} | {10} | {0} | {7} | {27} |\n"
        "^---v---^---v---^---v---^---v---^---v---^\n"
        "    | {34} | {5} | {6} | {26} |\n"
        "    ^---v---^---v---^---v---^---v---^\n"
        "        | {23} | {24} | {25} |\n"
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
        case Rule::RULE_CELL_INVALID_OFFSET:
            assert(false && "rule cell should not have INVALID_OFFSET state");
            break;
        }
    }

    return fmt.format(fields);
}
