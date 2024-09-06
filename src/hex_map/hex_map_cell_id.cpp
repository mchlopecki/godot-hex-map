#include "hex_map_cell_id.h"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/string.hpp"
#include "hex_map_cell_iterator.h"

const HexMapCellId HexMapCellId::Origin(0.0f, 0.0f, 0.0f);

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

HexMapCellIterator HexMapCellId::get_neighbors(
		unsigned int radius, const HexMap::Planes &planes) const {
	return HexMapCellIterator(*this, radius, planes);
}

HexMapCellId::operator String() const {
	// clang-format off
	return (String)"{ .q = " + itos(q) +
		", .r = " + itos(r) +
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
