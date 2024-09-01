#include "editor_cursor.h"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/mesh_library.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <cassert>

void EditorCursor::free_meshes() {
	RenderingServer *rs = RS::get_singleton();
	for (const CursorCell &cell : tiles) {
		if (cell.mesh_instance.is_valid()) {
			rs->free_rid(cell.mesh_instance);
		}
	}
}

void EditorCursor::clear_tiles() {
	free_meshes();
	tiles.clear();
	orientation = TileOrientation::Upright0;
}

void EditorCursor::set_tile(
		CellId cell,
		int tile,
		TileOrientation orientation) {
	RenderingServer *rs = RS::get_singleton();

	// remove any duplicate cell in the existing cursor
	for (auto it = tiles.front(); it != nullptr; it = it->next()) {
		if (it->get().cell_id != cell) {
			continue;
		}
		rs->free_rid(it->get().mesh_instance);
		tiles.erase(it);
		break;
	}

	if (tile == -1) {
		return;
	}

	CursorCell cc = {
		.cell_id = cell,
		.tile = tile,
		.orientation = orientation
	};
	Ref<MeshLibrary> mesh_library = hex_map->get_mesh_library();
	RID scenario = hex_map->get_window()->get_world_3d()->get_scenario();
	RID mesh = mesh_library->get_item_mesh(tile)->get_rid();
	cc.mesh_instance = rs->instance_create2(mesh, scenario);
	tiles.push_back(cc);

	// XXX need to figure out a way to draw cursor cells over hexmap cells
}

void EditorCursor::transform_cell_meshes() {
	RenderingServer *rs = RS::get_singleton();
	Ref<MeshLibrary> mesh_library = hex_map->get_mesh_library();

	for (CursorCell &cell : tiles) {
		Transform3D transform;
		transform.origin = hex_map->map_to_local(pointer_cell);
		transform.basis = (Basis)this->orientation;
		transform.translate_local(hex_map->map_to_local(cell.cell_id));

		// apply the tile rotation
		transform.basis = (Basis)cell.orientation * transform.basis;

		cell.cell_id_live = hex_map->local_to_map(transform.origin);

		transform *= mesh_library->get_item_mesh_transform(cell.tile);

		rs->instance_set_transform(cell.mesh_instance, transform);
	}
}

List<EditorCursor::CursorCell> EditorCursor::get_tiles() {
	RenderingServer *rs = RS::get_singleton();
	List<CursorCell> out;

	for (const CursorCell &cell : tiles) {
		CursorCell cc = {
			.cell_id = cell.cell_id + pointer_cell,
			.cell_id_live = cell.cell_id_live,
			.tile = cell.tile,
			.orientation = cell.orientation + orientation,
		};
		out.push_back(cc);
	}

	return out;
}

void EditorCursor::hide() {
	RenderingServer *rs = RS::get_singleton();
	for (CursorCell &cell : tiles) {
		rs->instance_set_visible(cell.mesh_instance, false);
	}
}
void EditorCursor::show() {
	RenderingServer *rs = RS::get_singleton();
	for (CursorCell &cell : tiles) {
		rs->instance_set_visible(cell.mesh_instance, true);
	}
}

void EditorCursor::redraw() {
	transform_cell_meshes();
}

void EditorCursor::set_pointer(CellId cell) {
	if (cell == this->pointer_cell) {
		return;
	}
	this->pointer_cell = cell;
	transform_cell_meshes();
}

void EditorCursor::set_orientation(TileOrientation orientation) {
	this->orientation = orientation;
	transform_cell_meshes();
}

EditorCursor::EditorCursor(HexMap *map) {
	hex_map = map;
}

EditorCursor::~EditorCursor() {
	free_meshes();
}
