#include "hex_map_cell_id.h"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "hex_map_iter_axial.h"

const HexMapCellId HexMapCellId::Origin(0.0f, 0.0f, 0.0f);

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

HexMapIterAxial HexMapCellId::get_neighbors(
		unsigned int radius, const HexMap::Planes &planes) const {
	return HexMapIterAxial(*this, radius, planes);
}

HexMapCellId::operator String() const {
	// clang-format off
	return (String)"{ .q = " + itos(q) +
		", .r = " + itos(r) +
		", .s = " + itos(s()) +
		", .y = " + itos(y) + "}";
	// clang-format on
}

std::ostream &operator<<(std::ostream &os, const HexMapCellId &value) {
	os << "{ .q = ";
	os << value.q;
	os << ", .r = ";
	os << value.r;
	os << ", .y = ";
	os << value.y;
	os << " }";
	return os;
}

Ref<HexMapCellIdRef> HexMapCellIdRef::operator+(
		const HexMapCellId &delta) const {
	Ref<HexMapCellIdRef> ref;
	ref.instantiate();
	ref->cell_id = cell_id + delta;
	return ref;
}

void HexMapCellIdRef::_bind_methods() {
	// static constructors
	ClassDB::bind_static_method("HexMapCellIdRef",
			D_METHOD("from_values", "hash"), &HexMapCellIdRef::_from_values);
	ClassDB::bind_static_method("HexMapCellIdRef",
			D_METHOD("from_hash", "hash"), &HexMapCellIdRef::_from_hash);

	// field accessors
	ClassDB::bind_method(D_METHOD("get_q"), &HexMapCellIdRef::_get_q);
	ClassDB::bind_method(D_METHOD("get_r"), &HexMapCellIdRef::_get_r);
	ClassDB::bind_method(D_METHOD("get_s"), &HexMapCellIdRef::_get_s);
	ClassDB::bind_method(D_METHOD("get_y"), &HexMapCellIdRef::_get_y);

	// directional helpers
	ClassDB::bind_method(D_METHOD("east"), &HexMapCellIdRef::_east);
	ClassDB::bind_method(D_METHOD("northeast"), &HexMapCellIdRef::_northeast);
	ClassDB::bind_method(D_METHOD("northwest"), &HexMapCellIdRef::_northwest);
	ClassDB::bind_method(D_METHOD("west"), &HexMapCellIdRef::_west);
	ClassDB::bind_method(D_METHOD("southwest"), &HexMapCellIdRef::_southwest);
	ClassDB::bind_method(D_METHOD("southeast"), &HexMapCellIdRef::_southeast);
	ClassDB::bind_method(D_METHOD("up"), &HexMapCellIdRef::_up);
	ClassDB::bind_method(D_METHOD("down"), &HexMapCellIdRef::_down);

	ClassDB::bind_method(D_METHOD("hash"), &HexMapCellIdRef::_hash);
	ClassDB::bind_method(
			D_METHOD("equals", "other"), &HexMapCellIdRef::_equals);
	ClassDB::bind_method(D_METHOD("get_neighbors", "radius"),
			&HexMapCellIdRef::_get_neighbors, 1);
}

Ref<HexMapCellIdRef> HexMapCellIdRef::_from_hash(uint64_t p_hash) {
	Ref<HexMapCellIdRef> ref;
	ref.instantiate();
	ref->cell_id.q = p_hash >> 32;
	ref->cell_id.r = p_hash >> 16;
	ref->cell_id.y = p_hash;
	return ref;
}

Ref<HexMapIterAxial> HexMapCellIdRef::_get_neighbors(
		unsigned int radius) const {
	Ref<HexMapIterAxial> ref;
	ref.instantiate();
	ref->set(cell_id.get_neighbors(radius));
	return ref;
}
