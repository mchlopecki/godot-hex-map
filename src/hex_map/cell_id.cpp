#include <climits>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/string.hpp>

#include "cell_id.h"
#include "godot_cpp/variant/packed_vector3_array.hpp"
#include "iter_radial.h"

const HexMapCellId HexMapCellId::Origin(0, 0, 0);
const HexMapCellId HexMapCellId::Invalid(INT_MAX, INT_MAX, INT_MAX);

// based on blog post https://observablehq.com/@jrus/hexround
static inline HexMapCellId axial_round(real_t q_in, real_t r_in) {
    int q = round(q_in);
    int r = round(r_in);

    real_t q_rem = q_in - q;
    real_t r_rem = r_in - r;

    if (abs(q_rem) >= abs(r_rem)) {
        q += round(0.5 * r_rem + q_rem);
    } else {
        r += round(0.5 * q_rem + r_rem);
    }

    return HexMapCellId(q, r, 0);
}

HexMapCellId HexMapCellId::from_unit_point(const Vector3 &point) {
    // convert x/z point into axial hex coordinates
    // https://www.redblobgames.com/grids/hexagons/#pixel-to-hex
    real_t q = (Math_SQRT3 / 3 * point.x - 1.0 / 3 * point.z);
    real_t r = (2.0 / 3 * point.z);
    HexMapCellId cell_id = axial_round(q, r);

    cell_id.y = (int)point.y;

    return cell_id;
}

Vector3 HexMapCellId::unit_center() const {
    // convert axial hex coordinates to a point
    // https://www.redblobgames.com/grids/hexagons/#hex-to-pixel
    //
    // XXX the x/z coordinates are in the center of the cell, but the y
    // coordinate is at the bottom.  This should brobably be changed, but I'm
    // not sure what all this will break.  Another time.
    return Vector3((Math_SQRT3 * q + SQRT3_2 * r), y, (3.0 / 2 * r));
}

unsigned HexMapCellId::distance(const HexMapCellId &other) const {
    HexMapCellId delta = other - *this;

    unsigned hex_dist =
            (ABS(delta.q) + ABS(delta.q + delta.r) + ABS(delta.r)) / 2;
    unsigned y_dist = ABS(delta.y);

    // movement laterally can combined with vertical movement, so which ever
    // of the two distances is greater is the distance.
    return hex_dist + y_dist;
}

HexMapIterRadial HexMapCellId::get_neighbors(unsigned int radius,
        const HexMap::Planes &planes) const {
    return HexMapIterRadial(*this, radius, true, planes);
}

HexMapCellId::operator String() const {
    // clang-format off
    return (String)"{ .q = " + itos(q) +
                   ", .r = " + itos(r) +
                   ", .s = " + itos(s()) +
                   ", .y = " + itos(y) + "}";
    // clang-format on
}

void HexMapCellIdWrapper::_bind_methods() {
    // static constructors
    ClassDB::bind_static_method("HexMapCellId",
            D_METHOD("from_coordinates", "q", "r", "y"),
            &HexMapCellIdWrapper::_from_coordinates);
    ClassDB::bind_static_method("HexMapCellId",
            D_METHOD("from_vector", "vector"),
            &HexMapCellIdWrapper::_from_vector);
    ClassDB::bind_static_method("HexMapCellId",
            D_METHOD("from_int", "int"),
            &HexMapCellIdWrapper::_from_int);

    // type conversions
    ClassDB::bind_method(D_METHOD("as_int"), &HexMapCellIdWrapper::_as_int);
    ClassDB::bind_method(
            D_METHOD("as_vector"), &HexMapCellIdWrapper::_as_vector);

    // field accessors
    ClassDB::bind_method(D_METHOD("get_q"), &HexMapCellIdWrapper::_get_q);
    ClassDB::bind_method(D_METHOD("get_r"), &HexMapCellIdWrapper::_get_r);
    ClassDB::bind_method(D_METHOD("get_s"), &HexMapCellIdWrapper::_get_s);
    ClassDB::bind_method(D_METHOD("get_y"), &HexMapCellIdWrapper::_get_y);

    // directional helpers
    ClassDB::bind_method(D_METHOD("east"), &HexMapCellIdWrapper::_east);
    ClassDB::bind_method(
            D_METHOD("northeast"), &HexMapCellIdWrapper::_northeast);
    ClassDB::bind_method(
            D_METHOD("northwest"), &HexMapCellIdWrapper::_northwest);
    ClassDB::bind_method(D_METHOD("west"), &HexMapCellIdWrapper::_west);
    ClassDB::bind_method(
            D_METHOD("southwest"), &HexMapCellIdWrapper::_southwest);
    ClassDB::bind_method(
            D_METHOD("southeast"), &HexMapCellIdWrapper::_southeast);
    ClassDB::bind_method(D_METHOD("up"), &HexMapCellIdWrapper::_up);
    ClassDB::bind_method(D_METHOD("down"), &HexMapCellIdWrapper::_down);

    ClassDB::bind_method(
            D_METHOD("equals", "other"), &HexMapCellIdWrapper::_equals);
    ClassDB::bind_method(D_METHOD("get_neighbors", "radius"),
            &HexMapCellIdWrapper::_get_neighbors,
            1);
}

int HexMapCellIdWrapper::_get_q() { return cell_id.q; }

int HexMapCellIdWrapper::_get_r() { return cell_id.r; }

int HexMapCellIdWrapper::_get_s() { return cell_id.s(); }

int HexMapCellIdWrapper::_get_y() { return cell_id.y; }

Ref<HexMapCellIdWrapper>
HexMapCellIdWrapper::_from_coordinates(int p_q, int p_r, int p_y) {
    return CellId(p_q, p_r, p_y);
}

Vector3i HexMapCellIdWrapper::_as_vector() {
    return Vector3i(cell_id.q, cell_id.r, cell_id.y);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_from_vector(Vector3i p_vector) {
    return CellId(p_vector.x, p_vector.y, p_vector.z);
}

uint64_t HexMapCellIdWrapper::_as_int() {
    CellId::Key key(cell_id);
    return key.key;
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_from_int(uint64_t p_int) {
    CellId::Key key(p_int);
    return CellId(key);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_east() const {
    return *this + CellId(1, 0, 0);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_northeast() const {
    return *this + CellId(1, -1, 0);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_northwest() const {
    return *this + CellId(0, -1, 0);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_west() const {
    return *this + CellId(-1, 0, 0);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_southwest() const {
    return *this + CellId(-1, 1, 0);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_southeast() const {
    return *this + CellId(0, 1, 0);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_down() const {
    return *this + CellId(0, 0, -1);
}

Ref<HexMapCellIdWrapper> HexMapCellIdWrapper::_up() const {
    return *this + CellId(0, 0, 1);
}

String HexMapCellIdWrapper::_to_string() const { return (String)cell_id; };

bool HexMapCellIdWrapper::_equals(const Ref<HexMapCellIdWrapper> other) const {
    return cell_id == other->cell_id;
}

Ref<HexMapIterWrapper> HexMapCellIdWrapper::_get_neighbors(
        unsigned int radius) const {
    return cell_id.get_neighbors(radius);
}
