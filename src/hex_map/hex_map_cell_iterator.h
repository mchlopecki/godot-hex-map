#pragma once

#include "godot_cpp/variant/string.hpp"
#include "hex_map.h"
#include "hex_map_cell_id.h"

// XXX need to make this iterable in gdscript
// https://docs.godotengine.org/en/latest/tutorials/scripting/gdscript/gdscript_advanced.html#custom-iterators

// How do you want to iterate around cells?
// - all cells on y plane around center
// - all cells in 3d space around center
// - was trying all cells on Q, R, S planes
//		- not really useful for anything but the grid generator
//		- these cell coordinates are pretty easy to create, Q = N, R +/- 1

struct HexMapCellIterator {
public:
	HexMapCellIterator(const HexMapCellId center, unsigned int radius,
			const HexMap::Planes &planes = HexMap::Planes::All);

	HexMapCellId operator*() const { return cell; }
	const HexMapCellId &operator->() { return cell; }

	// Prefix increment
	HexMapCellIterator &operator++();

	// Postfix increment
	HexMapCellIterator operator++(int);

	friend bool operator==(
			const HexMapCellIterator &a, const HexMapCellIterator &b) {
		return a.cell == b.cell;
	};
	friend bool operator!=(
			const HexMapCellIterator &a, const HexMapCellIterator &b) {
		return a.cell != b.cell;
	};

	HexMapCellIterator begin();
	HexMapCellIterator end();

	operator String() const;
	// added for testing
	friend std::ostream &operator<<(
			std::ostream &os, const HexMapCellIterator &value);

private:
	unsigned int radius;
	int y_min, y_max;
	int q_min, q_max; // inclusive min/max
	int r_min, r_max;
	int s_min, s_max;
	HexMapCellId center, cell;
};
