#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "cell_id.h"
#include "hex_map_node.h"

void HexMapNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_space"), &HexMapNode::_get_space);
    ClassDB::bind_method(D_METHOD("set_space", "space"),
            static_cast<void (HexMapNode::*)(
                    const Ref<hex_bind::HexMapSpace> &)>(
                    &HexMapNode::set_space));

    ClassDB::bind_method(D_METHOD("set_cell_height", "height"),
            &HexMapNode::set_cell_height);
    ClassDB::bind_method(
            D_METHOD("get_cell_height"), &HexMapNode::get_cell_height);
    ClassDB::bind_method(D_METHOD("set_cell_radius", "radius"),
            &HexMapNode::set_cell_radius);
    ClassDB::bind_method(
            D_METHOD("get_cell_radius"), &HexMapNode::get_cell_radius);

    ClassDB::bind_method(
            D_METHOD("set_center_y", "enable"), &HexMapNode::set_center_y);
    ClassDB::bind_method(D_METHOD("get_center_y"), &HexMapNode::get_center_y);

    ClassDB::bind_method(D_METHOD("set_cell", "cell", "value", "orientation"),
            static_cast<void (HexMapNode::*)(
                    const Ref<hex_bind::HexMapCellId>, int, int)>(
                    &HexMapNode::set_cell),
            DEFVAL(0));
    ClassDB::bind_method(D_METHOD("set_cells", "cells"),
            static_cast<void (HexMapNode::*)(const Array)>(
                    &HexMapNode::set_cells));
    ClassDB::bind_method(D_METHOD("get_cells", "cells"),
            static_cast<Array (HexMapNode::*)(const Array)>(
                    &HexMapNode::get_cells));

    ClassDB::bind_method(D_METHOD("get_cell_vecs"),
            static_cast<Array (HexMapNode::*)() const>(
                    &HexMapNode::get_cell_vecs));
    ClassDB::bind_method(D_METHOD("find_cell_vecs_by_value", "value"),
            static_cast<Array (HexMapNode::*)(int) const>(
                    &HexMapNode::find_cell_vecs_by_value));

    ClassDB::bind_method(D_METHOD("get_cell_center", "cell_id"),
            static_cast<Vector3 (HexMapNode::*)(
                    const Ref<hex_bind::HexMapCellId>) const>(
                    &HexMapNode::get_cell_center));
    ClassDB::bind_method(D_METHOD("get_cell_ids_in_local_quad",
                                 "a",
                                 "b",
                                 "c",
                                 "d",
                                 "padding"),
            &HexMapNode::get_cell_ids_in_local_quad);

    ADD_GROUP("Cell", "cell_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT,
                         "cell_height",
                         PROPERTY_HINT_NONE,
                         "suffix:m"),
            "set_cell_height",
            "get_cell_height");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT,
                         "cell_radius",
                         PROPERTY_HINT_NONE,
                         "suffix:m"),
            "set_cell_radius",
            "get_cell_radius");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cell_center_y"),
            "set_center_y",
            "get_center_y");

    ADD_SIGNAL(MethodInfo("cell_scale_changed"));
    ADD_SIGNAL(MethodInfo("mesh_offset_changed"));
    ADD_SIGNAL(MethodInfo(
            "cells_changed", PropertyInfo(Variant::ARRAY, "cells")));
    // XXX add signal cell_changed; connect in auto-tiled node, apply rules

    BIND_CONSTANT(CellArrayWidth);
    BIND_CONSTANT(INVALID_CELL_VALUE);
}

void HexMapNode::_notification(int p_what) {
    switch (p_what) {
    case NOTIFICATION_POSTINITIALIZE:
        set_notify_transform(true);
        break;
    case NOTIFICATION_TRANSFORM_CHANGED:
        space.set_transform(get_global_transform());
        break;
    }
}

void HexMapNode::set_space(const HexMapSpace &space) {
    this->space = space;
    cell_scale_changed();
}

void HexMapNode::set_space(const Ref<hex_bind::HexMapSpace> &ref) {
    space = ref->inner;
    cell_scale_changed();
}

Ref<hex_bind::HexMapSpace> HexMapNode::_get_space() {
    Ref<hex_bind::HexMapSpace> ref(memnew(hex_bind::HexMapSpace(space)));
    return ref;
}

void HexMapNode::set_cell_height(real_t p_height) {
    space.set_cell_height(p_height);
    cell_scale_changed();
}

real_t HexMapNode::get_cell_height() const { return space.get_cell_height(); }

void HexMapNode::set_cell_radius(real_t p_radius) {
    space.set_cell_radius(p_radius);
    cell_scale_changed();
}

real_t HexMapNode::get_cell_radius() const { return space.get_cell_radius(); }

void HexMapNode::set_center_y(bool p_value) {
    if (p_value) {
        space.set_mesh_offset(Vector3(0, 0, 0));
    } else {
        space.set_mesh_offset(Vector3(0, -0.5, 0));
    }
    cell_scale_changed();
}

bool HexMapNode::get_center_y() const {
    return space.get_mesh_offset().y == 0;
}

bool HexMapNode::cell_scale_changed() {
    emit_signal("cell_scale_changed");
    return true;
}

Vector3 HexMapNode::get_cell_scale() const { return space.get_cell_scale(); }

Vector3 HexMapNode::get_cell_center(const HexMapCellId &cell_id) const {
    return space.get_cell_center(cell_id);
}

Vector3 HexMapNode::get_cell_center(
        const Ref<hex_bind::HexMapCellId> cell_id) const {
    return space.get_cell_center(**cell_id);
}

HexMapCellId HexMapNode::get_cell_id(Vector3 pos) const {
    return space.get_cell_id(pos);
}

void HexMapNode::set_cell(const Ref<hex_bind::HexMapCellId> cell_id,
        int p_item,
        int p_orientation) {
    set_cell(**cell_id, p_item, p_orientation);
    // XXX emit this here because it is the entry point for set single cell via
    // gdscript, but this feels like there's a gap in our signal coverage.
    //
    // FOUND IT!  HexMapNodeEditorPlugin::selection_move() calls set_cell()
    // directly, which bypasses the cells_changed signal.  This causes a
    // graphical artifact during move cells of an IntNode with a child
    // AutoTiledNode; the grabbed cells don't disappear from the screen because
    // the AutoTiledNode is never notified that the IntNode was changed.
    //
    // Yea, additional problems encountered during drag paint/erase; those
    // maybe should also trigger events.  Some event is necessary to have
    // autotiled show results with every cell changed in paint/erase.
    emit_signal("cells_changed",
            Array::make(cell_id->inner.to_vec(), p_item, p_orientation));
}

void HexMapNode::set_cells(const Array cells) {
    int size = cells.size();
    ERR_FAIL_COND_MSG(size % CellArrayWidth != 0,
            "set_cells(): Array size must be a multiple of " +
                    itos(CellArrayWidth));
    for (int i = 0; i < size; i += 3) {
        HexMapCellId cell_id(cells[i]);
        int value = cells[i + 1];
        HexMapTileOrientation orientation = cells[i + 2];
        set_cell(cell_id, value, orientation);
    }
    emit_signal("cells_changed", cells);
}

Array HexMapNode::get_cells(const Array cells) {
    int size = cells.size();
    Array out;
    out.resize(size * 3);
    for (int i = 0; i < size; i++) {
        HexMapCellId cell_id(cells[i]);
        auto [value, orientation] = get_cell(cell_id);
        int base = i * 3;
        out[base] = (Vector3i)cell_id;
        out[base + 1] = value;
        out[base + 2] = orientation;
    }
    return out;
}

void HexMapNode::set_cells_visibility(const Array cells) {
    int len = cells.size();
    for (int i = 0; i < len; i += 2) {
        set_cell_visibility((Vector3i)cells[i], cells[i + 1]);
    }
}

Array HexMapNode::get_cell_ids_in_local_quad(Vector3 a,
        Vector3 b,
        Vector3 c,
        Vector3 d,
        float padding) const {
    Vector<HexMapCellId> cell_ids =
            space.get_cell_ids_in_local_quad(a, b, c, d, padding);
    size_t count = cell_ids.size();
    Array out;
    out.resize(count);
    for (int i = 0; i < count; i++) {
        out[i] = Ref<hex_bind::HexMapCellId>(cell_ids[i]);
    }
    return out;
}
