#pragma once

#include "godot_cpp/variant/string.hpp"
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
	struct Planes {
		bool y : 1;
		bool q : 1;
		bool r : 1;
		bool s : 1;
		static const Planes All;
		static const Planes QRS;
		static const Planes YQR;
		static const Planes YRS;
		static const Planes YQS;
	};

	HexMapCellIterator(const HexMapCellId center, unsigned int radius,
			Planes planes = Planes::All);

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
