/**************************************************************************/
/*  grid_map.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "hex_map.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "hex_map/octant.h"
#include "hex_map_cell_id.h"
#include "profiling.h"
#include "tile_orientation.h"

#include "godot_cpp/variant/basis.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_library.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_server3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/physics_material.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/pair.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>

bool HexMap::_set(const StringName &p_name, const Variant &p_value) {
	String name = p_name;

	if (name == "data") {
		Dictionary d = p_value;

		if (d.has("cells")) {
			const PackedByteArray cells = d["cells"];
			ERR_FAIL_COND_V(cells.size() % 10 != 0, false);

			size_t offset = 0;
			while (offset < cells.size()) {
				CellId::Key key;
				key.q = cells.decode_s16(offset);
				offset += 2;
				key.r = cells.decode_s16(offset);
				offset += 2;
				key.y = cells.decode_s16(offset);
				offset += 2;

				Cell cell;
				cell.cell = cells.decode_u32(offset);
				offset += 4;

				cell_map[key] = cell;
			}
		}

		_recreate_octant_data();

	} else if (name == "baked_meshes") {
		clear_baked_meshes();

		Array meshes = p_value;

		// this will change to array[3], Ref<Mesh>
		for (int i = 0; i < meshes.size(); i++) {
			BakedMesh bm;
			bm.mesh = meshes[i];
			ERR_CONTINUE(!bm.mesh.is_valid());
			bm.instance = RS::get_singleton()->instance_create();
			RS::get_singleton()->instance_set_base(
					bm.instance, bm.mesh->get_rid());
			RS::get_singleton()->instance_attach_object_instance_id(
					bm.instance, get_instance_id());
			if (is_inside_tree()) {
				RS::get_singleton()->instance_set_scenario(
						bm.instance, get_world_3d()->get_scenario());
				RS::get_singleton()->instance_set_transform(
						bm.instance, get_global_transform());
			}
			baked_meshes.push_back(bm);
		}

		update_octant_meshes();

	} else {
		return false;
	}

	return true;
}

bool HexMap::_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;

	if (name == "data") {
		Dictionary d;

		PackedByteArray cells;
		cells.resize(cell_map.size() * 10);
		size_t offset = 0;
		for (const KeyValue<CellKey, Cell> &E : cell_map) {
			cells.encode_s16(offset, E.key.q);
			offset += 2;
			cells.encode_s16(offset, E.key.r);
			offset += 2;
			cells.encode_s16(offset, E.key.y);
			offset += 2;
			cells.encode_u32(offset, E.value.cell);
			offset += 4;
		}

		d["cells"] = cells;

		r_ret = d;
	} else if (name == "baked_meshes") {
		Array ret;
		ret.resize(baked_meshes.size());
		for (int i = 0; i < baked_meshes.size(); i++) {
			ret[i] = baked_meshes[i].mesh;
		}
		r_ret = ret;

	} else {
		return false;
	}

	return true;
}

void HexMap::_get_property_list(List<PropertyInfo> *p_list) const {
	if (baked_meshes.size()) {
		p_list->push_back(PropertyInfo(Variant::ARRAY,
				"baked_meshes",
				PROPERTY_HINT_NONE,
				"",
				PROPERTY_USAGE_STORAGE));
	}

	p_list->push_back(PropertyInfo(Variant::DICTIONARY,
			"data",
			PROPERTY_HINT_NONE,
			"",
			PROPERTY_USAGE_STORAGE));
}

void HexMap::set_collision_debug(bool value) {
	if (collision_debug == value) {
		return;
	}
	collision_debug = value;

	for (const auto &it : octants) {
		it.value->exit_world();
		it.value->enter_world();
	}
}

bool HexMap::get_collision_debug() const { return collision_debug; }

void HexMap::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	for (const auto &it : octants) {
		it.value->update_collision_properties();
	}
}

uint32_t HexMap::get_collision_layer() const { return collision_layer; }

void HexMap::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	for (const auto &it : octants) {
		it.value->update_collision_properties();
	}
}

uint32_t HexMap::get_collision_mask() const { return collision_mask; }

void HexMap::set_collision_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1,
			"Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(p_layer_number > 32,
			"Collision layer number must be between 1 and 32 inclusive.");
	uint32_t collision_layer_new = get_collision_layer();
	if (p_value) {
		collision_layer_new |= 1 << (p_layer_number - 1);
	} else {
		collision_layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(collision_layer_new);
}

bool HexMap::get_collision_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1,
			false,
			"Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(p_layer_number > 32,
			false,
			"Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void HexMap::set_collision_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1,
			"Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(p_layer_number > 32,
			"Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

void HexMap::set_collision_priority(real_t p_priority) {
	collision_priority = p_priority;
	for (const auto &it : octants) {
		it.value->update_collision_properties();
	}
}

real_t HexMap::get_collision_priority() const { return collision_priority; }

// PhysicsMaterial.computed_friction() & computed_bounce() not exposed in
// godot-cpp
#define computed_friction(mat)                                                \
	((mat)->is_rough() ? -mat->get_friction() : mat->get_friction())
#define computed_bounce(mat)                                                  \
	((mat)->is_absorbent() ? -mat->get_bounce() : mat->get_bounce())
void HexMap::set_physics_material(Ref<PhysicsMaterial> p_material) {
	physics_material = p_material;
	physics_body_friction = 1.0;
	physics_body_bounce = 0.0;

	if (physics_material.is_valid()) {
		physics_body_friction = computed_friction(physics_material);
		physics_body_bounce = computed_bounce(physics_material);
	}

	for (const auto &it : octants) {
		it.value->update_physics_params();
	}
}

Ref<PhysicsMaterial> HexMap::get_physics_material() const {
	return physics_material;
}

bool HexMap::get_collision_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1,
			false,
			"Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(p_layer_number > 32,
			false,
			"Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

void HexMap::set_mesh_library(const Ref<MeshLibrary> &p_mesh_library) {
	if (!mesh_library.is_null()) {
		mesh_library->disconnect(
				"changed", callable_mp(this, &HexMap::update_octant_meshes));
	}
	mesh_library = p_mesh_library;
	if (!mesh_library.is_null()) {
		mesh_library->connect(
				"changed", callable_mp(this, &HexMap::update_octant_meshes));
	}

	update_octant_meshes();
	emit_signal("mesh_library_changed");
}

Ref<MeshLibrary> HexMap::get_mesh_library() const { return mesh_library; }

void HexMap::set_cell_height(real_t value) {
	if (value != cell_height) {
		cell_height = value;
		update_octant_meshes();
		emit_signal("cell_size_changed");
	}
}

real_t HexMap::get_cell_height() const { return cell_height; }

void HexMap::set_cell_radius(real_t value) {
	if (value != cell_radius) {
		cell_radius = value;
		update_octant_meshes();
		emit_signal("cell_size_changed");
	}
}
real_t HexMap::get_cell_radius() const { return cell_radius; }

Vector3 HexMap::get_cell_scale() const {
	return Vector3(cell_radius, cell_height, cell_radius);
}

Vector3 HexMap::get_cell_mesh_offset() const {
	return Vector3(0, center_y ? cell_height / 2 : 0, 0);
}

void HexMap::set_octant_size(int p_size) {
	ERR_FAIL_COND(p_size == 0);
	octant_size = p_size;
	update_octant_meshes();
}

int HexMap::get_octant_size() const { return octant_size; }

void HexMap::set_center_y(bool p_enable) {
	center_y = p_enable;
	update_octant_meshes();
}

bool HexMap::get_center_y() const { return center_y; }

void HexMap::set_cell_item(const HexMapCellId &cell_id,
		int p_item,
		int p_rot) {
	auto prof = profiling_begin("set cell item");
	ERR_FAIL_COND_MSG(!cell_id.in_bounds(), "cell id is not in bounds");

	// Convert the cell id into a key.  If the cell is already set to the
	// specified values, just return.
	CellKey cell_key(cell_id);
	Cell *current_cell = cell_map.getptr(cell_key);
	if (current_cell != nullptr && p_item == current_cell->item &&
			p_rot == current_cell->rot) {
		return;
	}

	// look up the cell octant
	OctantKey octant_key(cell_id, octant_size);
	Octant **octant_ptr = octants.getptr(octant_key);
	Octant *octant = octant_ptr ? *octant_ptr : nullptr;

	if (p_item >= 0) {
		// set the cell
		Cell cell = {
			.item = static_cast<unsigned int>(p_item),
			.rot = static_cast<unsigned int>(p_rot),
		};
		cell_map.insert(cell_key, cell);
		updated_cells.insert(cell_key);

		// create a new octant if one doesn't already exist for this cell
		if (octant == nullptr) {
			octant = new Octant(*this);
			octants.insert(octant_key, octant);

			if (is_inside_tree()) {
				octant->enter_world();
			}
		}

		// add a cell to the octant, and schedule an update
		octant->add_cell(cell_key);
		update_dirty_octants();

	} else if (current_cell != nullptr) {
		// clear the cell
		cell_map.erase(cell_key);
		updated_cells.insert(cell_key);

		ERR_FAIL_COND_MSG(octant == nullptr, "octant for cell does not exist");
		octant->remove_cell(cell_key);
		update_dirty_octants();
	}

	return;
}

void HexMap::_set_cell_item(const Ref<HexMapCellIdWrapper> cell_id,
		int p_item,
		int p_rot) {
	ERR_FAIL_COND_MSG(!cell_id.is_valid(), "null cell id");
	set_cell_item(**cell_id, p_item, p_rot);
}

void HexMap::_set_cell_item_v(const Vector3i &cell_id, int p_item, int p_rot) {
	set_cell_item(cell_id, p_item, p_rot);
}

int HexMap::get_cell_item(const HexMapCellId &cell_id) const {
	ERR_FAIL_COND_V_MSG(
			!cell_id.in_bounds(), INVALID_CELL_ITEM, "cell id not in bounds");

	CellKey key(cell_id);

	if (!cell_map.has(key)) {
		return INVALID_CELL_ITEM;
	}
	return cell_map[key].item;
}

int HexMap::_get_cell_item(const Ref<HexMapCellIdWrapper> p_cell_id) const {
	return get_cell_item(**p_cell_id);
}

int HexMap::get_cell_item_orientation(const HexMapCellId &cell_id) const {
	ERR_FAIL_COND_V_MSG(!cell_id.in_bounds(), -1, "CellId out of bounds");

	CellKey key(cell_id);
	if (!cell_map.has(key)) {
		return -1;
	}
	return cell_map[key].rot;
}

int HexMap::_get_cell_item_orientation(
		const Ref<HexMapCellIdWrapper> p_cell_id) const {
	return get_cell_item_orientation(**p_cell_id);
}

// based on blog post https://observablehq.com/@jrus/hexround
static inline Vector2i axial_round(real_t q_in, real_t r_in) {
	int q = round(q_in);
	int r = round(r_in);

	real_t q_rem = q_in - q;
	real_t r_rem = r_in - r;

	if (abs(q_rem) >= abs(r_rem)) {
		q += round(0.5 * r_rem + q_rem);
	} else {
		r += round(0.5 * q_rem + r_rem);
	}

	return Vector2i(q, r);
}

// convert axial hex coordinates to offset coordinates
// https://www.redblobgames.com/grids/hexagons/#conversions-offset
static inline Vector3i axial_to_oddr(Vector3i axial) {
	int x = axial.x + (axial.z - (axial.z & 1)) / 2;
	return Vector3i(x, axial.y, axial.z);
}

static inline Vector3i oddr_to_axial(Vector3i oddr) {
	int q = oddr.x - (oddr.z - (oddr.z & 1)) / 2;
	return Vector3i(q, oddr.y, oddr.z);
}

HexMapCellId HexMap::local_to_cell_id(const Vector3 &local_position) const {
	Vector3 unit_pos = local_position / get_cell_scale();
	return HexMapCellId::from_unit_point(unit_pos);
}

Ref<HexMapCellIdWrapper> HexMap::_local_to_cell_id(
		const Vector3 &p_local_position) const {
	return local_to_cell_id(p_local_position);
}

Vector3 HexMap::cell_id_to_local(const HexMapCellId &cell_id) const {
	return cell_id.unit_center() * get_cell_scale() + get_cell_mesh_offset();
}

Vector3 HexMap::_cell_id_to_local(
		const Ref<HexMapCellIdWrapper> cell_id) const {
	return cell_id_to_local(**cell_id);
}

// Vector3 HexMap::map_to_local(const HexMapCellId &p_cell_id) const {
// 	Vector3 offset = _get_offset();
//
// 	// convert axial hex coordinates to a point
// 	// https://www.redblobgames.com/grids/hexagons/#hex-to-pixel
// 	Vector3 local;
// 	local.x = cell_size.x * (Math_SQRT3 * p_cell_id.q + SQRT3_2 *
// p_cell_id.r); 	local.y = p_cell_id.y * cell_size.y + offset.y; 	local.z
// = cell_size.x * (3.0 / 2 * p_cell_id.r); 	return local;
// }
Vector<HexMapCellId>
HexMap::local_region_to_map(Vector3 p_a, Vector3 p_b, Planes planes) const {
	Vector<HexMapCellId> cells;

	// XXX OddR Iterator, and Planes support

	// shuffle the fields of a & b around so that a is bottom-left, b is
	// top-right
	if (p_a.x > p_b.x) {
		SWAP(p_a.x, p_b.x);
	}
	if (p_a.y > p_b.y) {
		SWAP(p_a.y, p_b.y);
	}
	if (p_a.z > p_b.z) {
		SWAP(p_a.z, p_b.z);
	}
	Vector3i bottom_left = local_to_cell_id(p_a);
	Vector3i top_right = local_to_cell_id(p_b);

	// we need the x coordinate of the center of the corner cells
	// later. grab them now before we switch coordinate systems.
	real_t left_x_center = cell_id_to_local(bottom_left).x;
	real_t right_x_center = cell_id_to_local(top_right).x;

	// we're going to use a different coordinate system for this
	// operation.  It's much easier to walk the region when we use
	// offset coordinates.  So let's map our corners from axial to
	// offset, then walk the region the same as the square region.
	// We'll convert the coordinates back to axial before putting them
	// in the array.
	bottom_left = axial_to_oddr(bottom_left);
	top_right = axial_to_oddr(top_right);

	// Also, unlike square cells, the location of the corner of the
	// region within a cell matters for hex cells, specifically the x
	// coordinate.  If you pick a point anywhere within a hex cell,
	// and draw a line down along the z-axis, that line will intercept
	// either the cell to the southwest or southeast of the clicked
	// cell.
	//
	// For both the left and right sides of the region, we need to
	// determine which of the southwest/southeast cells fall within
	// the region.  We do this by adjusting the x-min and x-max for the
	// even and odd rows independently.  We use the following table to
	// determine the modifier for the rows for both the minimum x
	// value (in bottom_left.x), and the maximum x value (in
	// top_right.x).
	//
	// Given an x coordinate in local space:
	// | cell z coord | x > cell_center.x | odd mod | even mod |
	// | even         | false             |  -1     | 0        |
	// | even         | true              |   0     | 0        |
	// | odd          | false             |   0     | 0        |
	// | odd          | true              |   0     | 1        |

	// adjustment applied to the min x value for odd and even cells
	int x_min_delta[2] = { 0, 0 };

	// if we start on an odd row, and the region starts to the right
	// of center, we want to skip the even cells at x == a.x.
	if ((bottom_left.z & 1) == 1 && p_a.x > left_x_center) {
		x_min_delta[0] = 1;
	}
	// if we start on an even row, and the region starts to the left
	// of center, we want to include the odd cells at x = a.x - 1.
	else if ((bottom_left.z & 1) == 0 && p_a.x <= left_x_center) {
		x_min_delta[1] = -1;
	}

	// same as above, but for the max x values
	int x_max_delta[2] = { 0, 0 };
	if ((top_right.z & 1) == 1 && p_b.x > right_x_center) {
		x_max_delta[0] = 1;
	} else if ((top_right.z & 1) == 0 && p_b.x <= right_x_center) {
		x_max_delta[1] = -1;
	}
	for (int z = bottom_left.z; z <= top_right.z; z++) {
		for (int y = bottom_left.y; y <= top_right.y; y++) {
			int min_x = bottom_left.x + x_min_delta[z & 1];
			int max_x = top_right.x + x_max_delta[z & 1];
			for (int x = min_x; x <= max_x; x++) {
				Vector3i oddr = Vector3i(x, y, z);
				Vector3i axial = oddr_to_axial(oddr);
				cells.push_back(axial);
			}
		}
	}

	return cells;
}

TypedArray<Vector3i> HexMap::_local_region_to_map(Vector3 p_a,
		Vector3 p_b) const {
	TypedArray<Vector3i> out;
	for (const HexMapCellId &cell : local_region_to_map(p_a, p_b)) {
		out.append((Vector3i)cell);
	}
	return out;
}

void HexMap::_notification(int p_what) {
	static Transform3D last_transform;

	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE:
			// XXX workaround for connect during initialize
			break;

		case NOTIFICATION_ENTER_WORLD:
			for (auto &pair : octants) {
				pair.value->enter_world();
			}
			last_transform = get_global_transform();
			break;

		case NOTIFICATION_ENTER_TREE:
			_update_visibility();
			break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			Transform3D transform = get_global_transform();
			if (transform != last_transform) {
				// recalculate the octant cell transforms
				for (auto &it : octants) {
					it.value->update_transform();
				}
				last_transform = transform;
			}
			break;
		}
		case NOTIFICATION_EXIT_WORLD:
			for (auto &pair : octants) {
				pair.value->exit_world();
			}
			break;

		case NOTIFICATION_VISIBILITY_CHANGED:
			_update_visibility();
			break;
	}
}

void HexMap::_update_visibility() {
	if (!is_inside_tree()) {
		return;
	}

	for (auto &pair : octants) {
		pair.value->update_visibility();
	}
}

void HexMap::update_dirty_octants_callback() {
	ERR_FAIL_COND_MSG(!awaiting_update,
			"update_dirty_octants_callback() called unexpectedly");
	Vector<OctantKey> empty_octants;
	for (const auto pair : octants) {
		Octant *octant = pair.value;
		if (!octant->is_dirty()) {
			continue;
		}

		octant->update();
		if (octant->is_empty()) {
			empty_octants.push_back(pair.key);
		}
	}

	for (const auto key : empty_octants) {
		delete octants[key];
		octants.erase(key);
	}

	// XXX why is this needed here?
	_update_visibility();
	awaiting_update = false;

	if (!updated_cells.is_empty()) {
		Array cells;
		for (const CellKey &key : updated_cells) {
			cells.push_back((Vector3i)key);
		}
		updated_cells.clear();
		emit_signal("cells_changed", cells);
	}
}

void HexMap::update_dirty_octants() {
	if (awaiting_update) {
		return;
	}
	awaiting_update = true;
	callable_mp(this, &HexMap::update_dirty_octants_callback).call_deferred();
}

void HexMap::update_octant_meshes() {
	for (auto &it : octants) {
		it.value->update();
	}
}

void HexMap::_recreate_octant_data() {
	HashMap<CellKey, Cell> cell_copy = cell_map;
	_clear_internal();
	for (const KeyValue<CellKey, Cell> &E : cell_copy) {
		set_cell_item(CellId(E.key), E.value.item, E.value.rot);
	}
}

void HexMap::_clear_internal() {
	for (auto &octant_pair : octants) {
		Octant *octant = octant_pair.value;
		if (is_inside_tree()) {
			octant->exit_world();
		}
		delete octant;
	}
	octants.clear();
	cell_map.clear();
}

void HexMap::clear() {
	_clear_internal();
	clear_baked_meshes();
}

void HexMap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_collision_debug", "debug"),
			&HexMap::set_collision_debug);
	ClassDB::bind_method(
			D_METHOD("get_collision_debug"), &HexMap::get_collision_debug);

	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"),
			&HexMap::set_collision_layer);
	ClassDB::bind_method(
			D_METHOD("get_collision_layer"), &HexMap::get_collision_layer);

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"),
			&HexMap::set_collision_mask);
	ClassDB::bind_method(
			D_METHOD("get_collision_mask"), &HexMap::get_collision_mask);

	ClassDB::bind_method(
			D_METHOD("set_collision_mask_value", "layer_number", "value"),
			&HexMap::set_collision_mask_value);
	ClassDB::bind_method(D_METHOD("get_collision_mask_value", "layer_number"),
			&HexMap::get_collision_mask_value);

	ClassDB::bind_method(
			D_METHOD("set_collision_layer_value", "layer_number", "value"),
			&HexMap::set_collision_layer_value);
	ClassDB::bind_method(D_METHOD("get_collision_layer_value", "layer_number"),
			&HexMap::get_collision_layer_value);

	ClassDB::bind_method(D_METHOD("set_collision_priority", "priority"),
			&HexMap::set_collision_priority);
	ClassDB::bind_method(D_METHOD("get_collision_priority"),
			&HexMap::get_collision_priority);

	ClassDB::bind_method(D_METHOD("set_physics_material", "material"),
			&HexMap::set_physics_material);
	ClassDB::bind_method(
			D_METHOD("get_physics_material"), &HexMap::get_physics_material);

	ClassDB::bind_method(D_METHOD("set_mesh_library", "mesh_library"),
			&HexMap::set_mesh_library);
	ClassDB::bind_method(
			D_METHOD("get_mesh_library"), &HexMap::get_mesh_library);

	ClassDB::bind_method(
			D_METHOD("set_cell_height", "height"), &HexMap::set_cell_height);
	ClassDB::bind_method(
			D_METHOD("get_cell_height"), &HexMap::get_cell_height);
	ClassDB::bind_method(
			D_METHOD("set_cell_radius", "radius"), &HexMap::set_cell_radius);
	ClassDB::bind_method(
			D_METHOD("get_cell_radius"), &HexMap::get_cell_radius);

	ClassDB::bind_method(
			D_METHOD("set_octant_size", "size"), &HexMap::set_octant_size);
	ClassDB::bind_method(
			D_METHOD("get_octant_size"), &HexMap::get_octant_size);

	ClassDB::bind_method(
			D_METHOD("set_cell_item", "position", "item", "orientation"),
			&HexMap::_set_cell_item,
			DEFVAL(0));
	ClassDB::bind_method(
			D_METHOD("set_cell_item_v", "position", "item", "orientation"),
			&HexMap::_set_cell_item_v,
			DEFVAL(0));
	ClassDB::bind_method(
			D_METHOD("get_cell_item", "position"), &HexMap::_get_cell_item);
	ClassDB::bind_method(D_METHOD("get_cell_item_orientation", "position"),
			&HexMap::_get_cell_item_orientation);

	ClassDB::bind_method(
			D_METHOD("local_region_to_map", "local_point_a", "local_point_b"),
			&HexMap::_local_region_to_map);

	ClassDB::bind_method(D_METHOD("local_to_cell_id", "local_position"),
			&HexMap::_local_to_cell_id);
	ClassDB::bind_method(D_METHOD("cell_id_to_local", "cell_id"),
			&HexMap::_cell_id_to_local);

	ClassDB::bind_method(
			D_METHOD("set_center_y", "enable"), &HexMap::set_center_y);
	ClassDB::bind_method(D_METHOD("get_center_y"), &HexMap::get_center_y);

	ClassDB::bind_method(D_METHOD("clear"), &HexMap::clear);

	ClassDB::bind_method(D_METHOD("get_used_cells"), &HexMap::get_used_cells);
	ClassDB::bind_method(D_METHOD("get_used_cells_by_item", "item"),
			&HexMap::get_used_cells_by_item);

	ClassDB::bind_method(
			D_METHOD("get_bake_meshes"), &HexMap::get_bake_meshes);
	ClassDB::bind_method(D_METHOD("get_bake_mesh_instance", "idx"),
			&HexMap::get_bake_mesh_instance);
	ClassDB::bind_method(
			D_METHOD("clear_baked_meshes"), &HexMap::clear_baked_meshes);

	ClassDB::bind_method(D_METHOD("make_baked_meshes",
								 "gen_lightmap_uv",
								 "lightmap_uv_texel_size"),
			&HexMap::make_baked_meshes,
			DEFVAL(false),
			DEFVAL(0.1));

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
						 "mesh_library",
						 PROPERTY_HINT_RESOURCE_TYPE,
						 "MeshLibrary"),
			"set_mesh_library",
			"get_mesh_library");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
						 "physics_material",
						 PROPERTY_HINT_RESOURCE_TYPE,
						 "PhysicsMaterial"),
			"set_physics_material",
			"get_physics_material");
	ADD_GROUP("Cell", "cell_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT,
						 "cell_height",
						 PROPERTY_HINT_NONE,
						 "suffix:m"),
			"set_cell_height",
			"get_cell_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT,
						 "cell_radius",
						 PROPERTY_HINT_NONE,
						 "suffix:m"),
			"set_cell_radius",
			"get_cell_radius");
	ADD_PROPERTY(PropertyInfo(Variant::INT,
						 "cell_octant_size",
						 PROPERTY_HINT_RANGE,
						 "1,1024,1"),
			"set_octant_size",
			"get_octant_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cell_center_y"),
			"set_center_y",
			"get_center_y");
	ADD_GROUP("Collision", "collision_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL,
						 "collision_debug",
						 PROPERTY_HINT_LAYERS_3D_PHYSICS),
			"set_collision_debug",
			"get_collision_debug");
	ADD_PROPERTY(PropertyInfo(Variant::INT,
						 "collision_layer",
						 PROPERTY_HINT_LAYERS_3D_PHYSICS),
			"set_collision_layer",
			"get_collision_layer");
	ADD_PROPERTY(PropertyInfo(Variant::INT,
						 "collision_mask",
						 PROPERTY_HINT_LAYERS_3D_PHYSICS),
			"set_collision_mask",
			"get_collision_mask");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_priority"),
			"set_collision_priority",
			"get_collision_priority");

	BIND_CONSTANT(INVALID_CELL_ITEM);

	ADD_SIGNAL(MethodInfo("cell_size_changed"));
	ADD_SIGNAL(MethodInfo("mesh_library_changed"));
	ADD_SIGNAL(MethodInfo(
			"cell_changed", PropertyInfo(Variant::VECTOR3I, "cell")));
	ADD_SIGNAL(MethodInfo(
			"cells_changed", PropertyInfo(Variant::ARRAY, "cell_id_vectors")));
}

Array HexMap::get_used_cells() const {
	Array a;
	a.resize(cell_map.size());
	int i = 0;
	for (const KeyValue<CellKey, Cell> &E : cell_map) {
		HexMapCellId cell_id(E.key);
		a[i++] = static_cast<Ref<HexMapCellIdWrapper>>(cell_id);
	}

	return a;
}

TypedArray<Vector3i> HexMap::get_used_cells_by_item(int p_item) const {
	Array a;
	for (const KeyValue<CellKey, Cell> &E : cell_map) {
		if ((int)E.value.item == p_item) {
			HexMapCellId cell_id(E.key);
			a.push_back(static_cast<Ref<HexMapCellIdWrapper>>(cell_id));
		}
	}

	return a;
}

void HexMap::clear_baked_meshes() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	for (int i = 0; i < baked_meshes.size(); i++) {
		RS::get_singleton()->free_rid(baked_meshes[i].instance);
	}
	baked_meshes.clear();

	update_octant_meshes();
}

void HexMap::make_baked_meshes(bool p_gen_lightmap_uv,
		float p_lightmap_uv_texel_size) {}

Array HexMap::get_bake_meshes() {
	if (!baked_meshes.size()) {
		make_baked_meshes(true);
	}

	Array arr;
	for (int i = 0; i < baked_meshes.size(); i++) {
		arr.push_back(baked_meshes[i].mesh);
		arr.push_back(Transform3D());
	}

	return arr;
}

RID HexMap::get_bake_mesh_instance(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, baked_meshes.size(), RID());
	return baked_meshes[p_idx].instance;
}

bool HexMap::generate_navigation_source_geometry(Ref<NavigationMesh>,
		Ref<NavigationMeshSourceGeometryData3D> source_geometry_data,
		Node *) const {
	UtilityFunctions::print("navigation source generator");

	for (const auto &it : cell_map) {
		const Cell &cell = it.value;

		Ref<Mesh> mesh = mesh_library->get_item_mesh(cell.item);
		if (!mesh.is_valid()) {
			continue;
		}

		Transform3D cell_transform;
		cell_transform.basis = cell.get_basis();
		cell_transform.set_origin(cell_id_to_local(it.key));
		cell_transform *= mesh_library->get_item_mesh_transform(cell.item);

		source_geometry_data->add_mesh(mesh, cell_transform);
	}

	// Unused return value.  To turn this function into a Callable, the return
	// type needs to be able to be converted into a Variant.  `void` is not a
	// supported option.
	return true;
}

HexMap::HexMap() {
	set_notify_transform(true);

	// copied from SceneTree::get_debug_collision_material()
	collision_debug_mat.instantiate();
	collision_debug_mat->set_shading_mode(
			StandardMaterial3D::SHADING_MODE_UNSHADED);
	collision_debug_mat->set_transparency(
			StandardMaterial3D::TRANSPARENCY_ALPHA);
	collision_debug_mat->set_flag(
			StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	collision_debug_mat->set_flag(
			StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	collision_debug_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	collision_debug_mat->set_albedo(
			ProjectSettings::get_singleton()->get_setting_with_override(
					"debug/shapes/collision/shape_color"));

	// set up the source geometry parser for navmesh generation
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	navigation_source_geometry_parser = ns->source_geometry_parser_create();
	ns->source_geometry_parser_set_callback(navigation_source_geometry_parser,
			callable_mp(this, &HexMap::generate_navigation_source_geometry));
}

HexMap::~HexMap() {
	clear();
	NavigationServer3D::get_singleton()->free_rid(
			navigation_source_geometry_parser);
}
