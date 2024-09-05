#include "hex_map_cell_iterator.h"

HexMapCellIterator::HexMapCellIterator(const HexMapCellId center,
		unsigned int radius, const HexMap::Planes &planes) :
		center(center), radius(radius) {
	if (planes.y) {
		y_min = center.y - radius;
		y_max = center.y + radius;
	} else {
		y_min = y_max = center.y;
	}
	if (planes.q) {
		q_min = center.q - radius;
		q_max = center.q + radius;
	} else {
		q_min = q_max = center.q;
	}
	if (planes.r) {
		r_min = center.r - radius;
		r_max = center.r + radius;
	} else {
		r_min = r_max = center.r;
	}
	if (planes.s) {
		s_min = center.s() - radius;
		s_max = center.s() + radius;
	} else {
		s_min = s_max = center.s();
	}

	cell = HexMapCellId(q_min, r_min, y_min);
	if (center.distance(cell) > radius) {
		operator++();
	}
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
			cell.q = cell.r = cell.y = (radius + 1);
		} else {
			break;
		}
	} while (cell.s() < s_min || cell.s() > s_max ||
			center.distance(cell) > radius);
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
	++iter;
	return iter;
}

HexMapCellIterator HexMapCellIterator::end() {
	HexMapCellIterator iter = *this;
	iter.cell = HexMapCellId(radius + 1, radius + 1, radius + 1);
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
		".s = [" + itos(s_min) + ".." + itos(s_max) + "], " +
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
	os << "], .s = [";
	os << value.s_min;
	os << ", ";
	os << value.s_max;
	os << "], .y = [";
	os << value.y_min;
	os << ", ";
	os << value.y_max;
	os << "] }";

	return os;
}
