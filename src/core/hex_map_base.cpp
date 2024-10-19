#include "hex_map_base.h"
#include "cell_id.h"

void HexMapBase::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_cell_height", "height"),
            &HexMapBase::set_cell_height);
    ClassDB::bind_method(
            D_METHOD("get_cell_height"), &HexMapBase::get_cell_height);
    ClassDB::bind_method(D_METHOD("set_cell_radius", "radius"),
            &HexMapBase::set_cell_radius);
    ClassDB::bind_method(
            D_METHOD("get_cell_radius"), &HexMapBase::get_cell_radius);

    ClassDB::bind_method(
            D_METHOD("set_center_y", "enable"), &HexMapBase::set_center_y);
    ClassDB::bind_method(D_METHOD("get_center_y"), &HexMapBase::get_center_y);

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
    ADD_SIGNAL(MethodInfo("mesh_library_changed"));
}

void HexMapBase::set_cell_height(real_t p_height) {
    space.set_cell_height(p_height);
    cell_scale_changed();
}

real_t HexMapBase::get_cell_height() const { return space.get_cell_height(); }

void HexMapBase::set_cell_radius(real_t p_radius) {
    space.set_cell_radius(p_radius);
    cell_scale_changed();
}

real_t HexMapBase::get_cell_radius() const { return space.get_cell_radius(); }

void HexMapBase::set_center_y(bool p_value) {
    if (p_value) {
        space.set_mesh_offset(Vector3(0, 0, 0));
    } else {
        space.set_mesh_offset(Vector3(0, -0.5, 0));
    }
    cell_scale_changed();
}

bool HexMapBase::get_center_y() const {
    return space.get_mesh_offset().y == 0;
}

bool HexMapBase::cell_scale_changed() {
    emit_signal("cell_scale_changed");
    return true;
}

Vector3 HexMapBase::get_cell_scale() const { return space.get_cell_scale(); }

Vector3 HexMapBase::get_cell_center(const HexMapCellId &cell_id) const {
    return space.get_cell_center(cell_id);
}

HexMapCellId HexMapBase::get_cell_id(Vector3 pos) const {
    return space.get_cell_id(pos);
}
