#pragma once

#include "godot_cpp/variant/vector3.hpp"
#include "hex_map.h"

using namespace godot;

class HexMapIterAxial;

class HexMapCellId {
public:
	// axial coordinates
	int q, r;

	// y coordinate
	int y;

	HexMapCellId(int q = 0, int r = 0, int y = 0) : q(q), r(r), y(y){};
	HexMapCellId(Vector3i v) : q(v.x), r(v.z), y(v.y){};
	int s() const { return -q - r; }

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

	// calculate the distance between two cells in cell units
	unsigned distance(const HexMapCellId &) const;

	// get all cells within radius of this cell, along the provided planes
	HexMapIterAxial get_neighbors(unsigned int radius = 1,
			const HexMap::Planes &planes = HexMap::Planes::All) const;

	// get the pixel center of this cell assuming the cell is a unit cell with
	// height = 1, radius = 1.  Use HexMap.map_to_local() for center scaled
	// by cell size.
	Vector3 unit_center() const;

	static const HexMapCellId Origin;
};

// added for testing
std::ostream &operator<<(std::ostream &os, const HexMapCellId &value);
