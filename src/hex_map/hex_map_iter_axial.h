#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/string.hpp"
#include "hex_map.h"
#include "hex_map_cell_id.h"

// HexMap cell iterator using axial coordinates to define a volume to iterate
struct HexMapIterAxial {
public:
	HexMapIterAxial(const HexMapCellId center, unsigned int radius,
			const HexMap::Planes &planes = HexMap::Planes::All);

	inline operator Ref<HexMapIterAxialRef>() const;
	operator String() const;

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

	// added for testing
	friend std::ostream &operator<<(
			std::ostream &os, const HexMapIterAxial &value);

private:
	unsigned int radius;
	int y_min, y_max;
	int q_min, q_max;
	int r_min, r_max;
	int s_min, s_max;
	HexMapCellId center, cell;
};

// GDScript wrapper for HexMapIterAxial
class HexMapIterAxialRef : public godot::RefCounted {
	GDCLASS(HexMapIterAxialRef, RefCounted);

public:
	HexMapIterAxialRef() : iter(HexMapIterAxial(HexMapCellId::Origin, 0)){};
	HexMapIterAxialRef(const HexMapIterAxial &iter) : iter(iter){};

	// GDScript functions
	String _to_string() const;

	// GDScript custom iterator functions
	bool _iter_init(Variant _arg);
	bool _iter_next(Variant _arg);
	Ref<HexMapCellIdRef> _iter_get(Variant _arg);

protected:
	static void _bind_methods();

private:
	HexMapIterAxial iter;
};

inline HexMapIterAxial::operator Ref<HexMapIterAxialRef>() const {
	return Ref<HexMapIterAxialRef>(memnew(HexMapIterAxialRef(*this)));
}
