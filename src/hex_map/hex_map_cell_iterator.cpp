#include "hex_map_cell_iterator.h"

HexMapCellIterator::HexMapCellIterator(
		const HexMapCellId center, unsigned int radius) :
		center(center), radius(radius) {
	y_min = center.y - radius;
	y_max = center.y + radius;
	q_min = center.q - radius;
	q_max = center.q + radius;
	r_min = center.r - radius;
	r_max = center.r + radius;
	cell = HexMapCellId(q_min, r_min, y_min);
}

// prefix increment
HexMapCellIterator &HexMapCellIterator::operator++() {
	do {
		if (cell.r < r_max) {
			cell.r++;
		} else if (cell.q < q_max) {
			cell.r = r_min;
			cell.q++;
		} else if (cell.y < y_max) {
			cell.r = r_min;
			cell.q = q_min;
			cell.y++;
		} else if (cell.y == y_max) {
			cell.q = q_max + 1;
			cell.r = r_max + 1;
			cell.y = y_max + 1;
		} else {
			break;
		}
	} while (center.distance(cell) > radius);
	return *this;
}

// postfix increment
HexMapCellIterator HexMapCellIterator::operator++(int) {
	auto tmp = *this;
	++(*this);
	return tmp;
}

HexMapCellIterator HexMapCellIterator::begin() {
	HexMapCellIterator iter = *this;
	iter.cell = HexMapCellId(q_min, r_min, y_min);
	return iter;
}

HexMapCellIterator HexMapCellIterator::end() {
	HexMapCellIterator iter = *this;
	iter.cell = HexMapCellId(q_max + 1, r_max + 1, y_max + 1);
	return iter;
}

HexMapCellIterator::operator String() const {
	// clang-format off
	return (String)"{ " +
		".center = " + center.operator String() + ", " +
		".radius = " + itos(radius) + ", " +
		".cell = " + cell.operator String() + ", " +
		".q = [" + itos(q_min) + ".." + itos(q_max) + "], " +
		".r = [" + itos(r_min) + ".." + itos(r_max) + "], " +
		".y = [" + itos(y_min) + ".." + itos(y_max) + "], " +
	"}";
	// clang-format on
}

std::ostream &operator<<(std::ostream &os, const HexMapCellIterator &value) {
	os << "{ .center = ";
	os << value.center;
	os << ", .radius = ";
	os << value.radius;
	os << ", . cell = ";
	os << value.cell;
	os << ", .q = [";
	os << value.q_min;
	os << ", ";
	os << value.q_max;
	os << "], .r = [";
	os << value.r_min;
	os << ", ";
	os << value.r_max;
	os << "], .y = [";
	os << value.y_min;
	os << ", ";
	os << value.y_max;
	os << "] }";

	return os;
}
