#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "hex_map.h"
#include <cstdint>

using namespace godot;

class HexMapIterAxial;

class HexMapCellId {
public:
	// axial coordinates
	int16_t q, r;

	// y coordinate
	int16_t y;

	HexMapCellId() : q(0), r(0), y(0){};
	HexMapCellId(int q, int r, int y) : q(q), r(r), y(y){};
	HexMapCellId(Vector3i v) : q(v.x), r(v.z), y(v.y){};

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

	int s() const { return -q - r; }

	// calculate the distance between two cells in cell units
	unsigned distance(const HexMapCellId &) const;

	// get all cells within radius of this cell, along the provided planes
	HexMapIterAxial get_neighbors(unsigned int radius = 1,
			const HexMap::Planes &planes = HexMap::Planes::All) const;

	// get the pixel center of this cell assuming the cell is a unit cell with
	// height = 1, radius = 1.  Use HexMap.map_to_local() for center scaled
	// by cell size.
	Vector3 unit_center() const;

	// get a cell id for a point on a unit hex grid (height=1, radius-1)
	static HexMapCellId from_unit_point(const Vector3 &point);

	static const HexMapCellId Origin;
};

// added for testing
std::ostream &operator<<(std::ostream &os, const HexMapCellId &value);

// wrapper to return a HexMapCellId to GDscript
class HexMapCellIdRef : public RefCounted {
	GDCLASS(HexMapCellIdRef, RefCounted)

	HexMapCellId cell_id;

public:
	HexMapCellIdRef(const HexMapCellId &cell_id) : cell_id(cell_id){};
	HexMapCellIdRef(){};

	// c++ helpers
	const HexMapCellId &inner() const { return cell_id; }
	Ref<HexMapCellIdRef> operator+(const HexMapCellId &delta) const;

	// GDScript static constructors
	static Ref<HexMapCellIdRef> _from_values(
			int16_t p_q, int16_t p_r, int16_t p_y) {
		Ref<HexMapCellIdRef> ref;
		ref.instantiate();
		ref->cell_id = HexMapCellId(p_q, p_r, p_y);
		return ref;
	}
	static Ref<HexMapCellIdRef> _from_hash(uint64_t p_hash);

	// GDScript field accessors
	uint16_t _get_q() { return cell_id.q; }
	uint16_t _get_r() { return cell_id.r; }
	uint16_t _get_s() { return cell_id.s(); }
	uint16_t _get_y() { return cell_id.y; }

	// gdscript does not support equality for RefCounted objects.  We need some
	// way to use cell ids in arrays, hashes, etc.  We provide the hash method
	// for generating a type that gdscript can handle for equality.
	uint64_t _hash() {
		return (uint64_t)cell_id.q << 32 | cell_id.r << 16 | cell_id.y;
	}

	// directional helpers
	Ref<HexMapCellIdRef> _east() const {
		return *this + HexMapCellId(1, 0, 0);
	};
	Ref<HexMapCellIdRef> _northeast() const {
		return *this + HexMapCellId(1, -1, 0);
	};
	Ref<HexMapCellIdRef> _northwest() const {
		return *this + HexMapCellId(0, -1, 0);
	};
	Ref<HexMapCellIdRef> _west() const {
		return *this + HexMapCellId(-1, 0, 0);
	};
	Ref<HexMapCellIdRef> _southwest() const {
		return *this + HexMapCellId(-1, 1, 0);
	};
	Ref<HexMapCellIdRef> _southeast() const {
		return *this + HexMapCellId(0, 1, 0);
	};
	Ref<HexMapCellIdRef> _down() const {
		return *this + HexMapCellId(0, 0, -1);
	};
	Ref<HexMapCellIdRef> _up() const { return *this + HexMapCellId(0, 0, 1); };

	String _to_string() { return (String)cell_id; };
	bool _equals(const Ref<HexMapCellIdRef> other) const {
		return cell_id == other->cell_id;
	}
	Ref<HexMapIterAxial> _get_neighbors(unsigned int radius = 1) const;

protected:
	static void _bind_methods();
};
