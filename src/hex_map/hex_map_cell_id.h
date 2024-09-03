#pragma once

#include "editor/editor_control.h"

using namespace godot;

class HexMapCellIterator;

class HexMapCellId {
public:
	// axial coordinates
	int q, r;

	// y coordinate
	int y;

	HexMapCellId(int q = 0, int r = 0, int y = 0) : q(q), r(r), y(y){};

	// XXX remove this; temporary until we fully switch from Vector3i to
	// HexMapCellId
	operator Vector3i() const { return Vector3i(q, y, r); }
	operator String() const;

	HexMapCellId operator+(const HexMapCellId &other) const {
		return HexMapCellId(q + other.q, r + other.r, y + other.y);
	}
	HexMapCellId operator-(const HexMapCellId &other) const {
		return HexMapCellId(q - other.q, r - other.r, y - other.y);
	}
	friend bool operator==(const HexMapCellId &a, const HexMapCellId &b) {
		return a.q == b.q && a.r == b.r && a.y == b.y;
	}
	friend bool operator!=(const HexMapCellId &a, const HexMapCellId &b) {
		return a.q != b.q || a.r != b.r || a.y != b.y;
	}

	unsigned distance(const HexMapCellId &) const;
	HexMapCellIterator get_neighbors(unsigned int radius = 1,
			EditorControl::EditAxis axis =
					EditorControl::EditAxis::AXIS_Y) const;

	static const HexMapCellId Origin;
};

// added for testing
std::ostream &operator<<(std::ostream &os, const HexMapCellId &value);
