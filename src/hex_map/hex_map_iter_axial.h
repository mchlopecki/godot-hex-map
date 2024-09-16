#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "hex_map.h"
#include "hex_map_cell_id.h"

// XXX need to make this iterable in gdscript
// https://docs.godotengine.org/en/latest/tutorials/scripting/gdscript/gdscript_advanced.html#custom-iterators
// XXX make a separate class `HexMapIterAxialRef` to be held inside a `Ref`
// and returned to the client.

// How do you want to iterate around cells?
// - all cells on y plane around center
// - all cells in 3d space around center
// - was trying all cells on Q, R, S planes
//		- not really useful for anything but the grid generator
//		- these cell coordinates are pretty easy to create, Q = N, R +/- 1

struct HexMapIterAxial : public godot::RefCounted {
	GDCLASS(HexMapIterAxial, godot::RefCounted)

public:
	HexMapIterAxial(const HexMapCellId center, unsigned int radius,
			const HexMap::Planes &planes = HexMap::Planes::All);
	HexMapIterAxial() : HexMapIterAxial(HexMapCellId::Origin, 1) {}

	// c++ iterator functions
	HexMapCellId operator*() const { return cell; }
	const HexMapCellId &operator->() { return cell; }
	HexMapIterAxial &operator++();
	HexMapIterAxial operator++(int);

	friend bool operator==(
			const HexMapIterAxial &a, const HexMapIterAxial &b) {
		return a.cell == b.cell;
	};
	friend bool operator!=(
			const HexMapIterAxial &a, const HexMapIterAxial &b) {
		return a.cell != b.cell;
	};

	HexMapIterAxial begin();
	HexMapIterAxial end();

	void set(const HexMapIterAxial &other);

	// Godot custom iterator functions
	bool _should_continue() { return *this != end(); }
	bool _iter_init(Variant _arg) {
		*this = begin();
		return _should_continue();
	}
	bool _iter_next(Variant _arg) {
		++*this;
		return _should_continue();
	}
	Ref<HexMapCellIdRef> _iter_get(Variant _arg) {
		return Ref<HexMapCellIdRef>(memnew(HexMapCellIdRef(cell)));
	}

	operator String() const;

	// added for testing
	friend std::ostream &operator<<(
			std::ostream &os, const HexMapIterAxial &value);

protected:
	static void _bind_methods();

private:
	unsigned int radius;
	int y_min, y_max;
	int q_min, q_max;
	int r_min, r_max;
	int s_min, s_max;
	HexMapCellId center, cell;
};
