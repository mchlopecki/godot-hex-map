#include <climits>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "cell_id.h"
#include "iter_radial.h"
#include "math.h"

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

    cell_id.y = round(point.y);

    return cell_id;
}

Vector3 HexMapCellId::unit_center() const {
    // convert axial hex coordinates to a point
    // https://www.redblobgames.com/grids/hexagons/#hex-to-pixel
    return Vector3((Math_SQRT3 * q + Math_SQRT3_2 * r), y, (3.0 / 2 * r));
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
        const HexMapPlanes &planes,
        bool include_center) const {
    return HexMapIterRadial(*this, radius, !include_center, planes);
}

HexMapCellId::operator String() const {
    // clang-format off
    return (String)"{ .q = " + itos(q) +
                   ", .r = " + itos(r) +
                   ", .y = " + itos(y) + "}";
    // clang-format on
}

void hex_bind::HexMapCellId::_bind_methods() {
    // static constructors
    ClassDB::bind_static_method("HexMapCellId",
            D_METHOD("from_coordinates", "q", "r", "y"),
            &hex_bind::HexMapCellId::from_coordinates);
    ClassDB::bind_static_method("HexMapCellId",
            D_METHOD("from_vector", "vector"),
            &hex_bind::HexMapCellId::from_vector);
    ClassDB::bind_static_method("HexMapCellId",
            D_METHOD("from_int", "int"),
            &hex_bind::HexMapCellId::from_int);

    // type conversions
    ClassDB::bind_method(D_METHOD("as_int"), &hex_bind::HexMapCellId::as_int);
    ClassDB::bind_method(
            D_METHOD("as_vector"), &hex_bind::HexMapCellId::as_vector);

    // field accessors
    ClassDB::bind_method(D_METHOD("get_q"), &hex_bind::HexMapCellId::get_q);
    ClassDB::bind_method(D_METHOD("get_r"), &hex_bind::HexMapCellId::get_r);
    ClassDB::bind_method(D_METHOD("get_s"), &hex_bind::HexMapCellId::get_s);
    ClassDB::bind_method(D_METHOD("get_y"), &hex_bind::HexMapCellId::get_y);

    // directional helpers
    ClassDB::bind_method(D_METHOD("east"), &hex_bind::HexMapCellId::east);
    ClassDB::bind_method(
            D_METHOD("northeast"), &hex_bind::HexMapCellId::northeast);
    ClassDB::bind_method(
            D_METHOD("northwest"), &hex_bind::HexMapCellId::northwest);
    ClassDB::bind_method(D_METHOD("west"), &hex_bind::HexMapCellId::west);
    ClassDB::bind_method(
            D_METHOD("southwest"), &hex_bind::HexMapCellId::southwest);
    ClassDB::bind_method(
            D_METHOD("southeast"), &hex_bind::HexMapCellId::southeast);
    ClassDB::bind_method(D_METHOD("up"), &hex_bind::HexMapCellId::up);
    ClassDB::bind_method(D_METHOD("down"), &hex_bind::HexMapCellId::down);

    ClassDB::bind_method(
            D_METHOD("equals", "other"), &hex_bind::HexMapCellId::equals);
    ClassDB::bind_method(
            D_METHOD("unit_center"), &hex_bind::HexMapCellId::unit_center);
    ClassDB::bind_method(D_METHOD("get_neighbors", "radius", "include_self"),
            &hex_bind::HexMapCellId::get_neighbors,
            1,
            false);

    ClassDB::bind_method(
            D_METHOD("add", "other"), &hex_bind::HexMapCellId::add);
    ClassDB::bind_method(
            D_METHOD("subtract", "other"), &hex_bind::HexMapCellId::subtract);
    ClassDB::bind_method(
            D_METHOD("reverse"), &hex_bind::HexMapCellId::reverse);
}

int hex_bind::HexMapCellId::get_q() { return inner.q; }

int hex_bind::HexMapCellId::get_r() { return inner.r; }

int hex_bind::HexMapCellId::get_s() { return inner.s(); }

int hex_bind::HexMapCellId::get_y() { return inner.y; }

Ref<hex_bind::HexMapCellId>
hex_bind::HexMapCellId::from_coordinates(int p_q, int p_r, int p_y) {
    return CellId(p_q, p_r, p_y);
}

Vector3i hex_bind::HexMapCellId::as_vector() { return (Vector3i)inner; }

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::from_vector(
        Vector3i p_vector) {
    return CellId(p_vector);
}

uint64_t hex_bind::HexMapCellId::as_int() {
    CellId::Key key(inner);
    return key.key;
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::from_int(uint64_t p_int) {
    CellId::Key key(p_int);
    return CellId(key);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::east() const {
    return inner + CellId(1, 0, 0);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::northeast() const {
    return inner + CellId(1, -1, 0);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::northwest() const {
    return inner + CellId(0, -1, 0);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::west() const {
    return inner + CellId(-1, 0, 0);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::southwest() const {
    return inner + CellId(-1, 1, 0);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::southeast() const {
    return inner + CellId(0, 1, 0);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::down() const {
    return inner + CellId(0, 0, -1);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::up() const {
    return inner + CellId(0, 0, 1);
}

String hex_bind::HexMapCellId::_to_string() const { return (String)inner; };

Vector3 hex_bind::HexMapCellId::unit_center() const {
    return inner.unit_center();
};

bool hex_bind::HexMapCellId::equals(
        const Ref<hex_bind::HexMapCellId> other) const {
    return inner == other->inner;
}

Ref<hex_bind::HexMapIter> hex_bind::HexMapCellId::get_neighbors(
        unsigned int radius,
        bool include_center) const {
    return inner.get_neighbors(radius, HexMapPlanes::All, include_center);
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::add(
        Ref<hex_bind::HexMapCellId> other) const {
    return inner + other->inner;
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::subtract(
        Ref<hex_bind::HexMapCellId> other) const {
    return inner - other->inner;
}

Ref<hex_bind::HexMapCellId> hex_bind::HexMapCellId::reverse() const {
    return inner * -1;
}
