#include "grid_manager.h"
#include "../hex_map.h"
#include "../hex_map_cell_id.h"
#include "../hex_map_cell_iterator.h"
#include "editor_control.h"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_plugin.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/sub_viewport.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#define GRID_RADIUS 5u

// create a mesh and draw a grid of hexagonal cells on it
void GridManager::build_y_grid() {
	Vector3 p_cell_size(hex_map->get_cell_size());

	// create the points that make up the top of a hex cell
	Vector<Vector3> shape_points;
	shape_points.append(Vector3(0.0, 0, -1.0) * p_cell_size);
	shape_points.append(Vector3(-SQRT3_2, 0, -0.5) * p_cell_size);
	shape_points.append(Vector3(-SQRT3_2, 0, 0.5) * p_cell_size);
	shape_points.append(Vector3(0.0, 0, 1.0) * p_cell_size);
	shape_points.append(Vector3(SQRT3_2, 0, 0.5) * p_cell_size);
	shape_points.append(Vector3(SQRT3_2, 0, -0.5) * p_cell_size);

	PackedVector3Array grid_points;
	for (const auto cell : HexMapCellId::Origin.get_neighbors(GRID_RADIUS)) {
		Vector3 center = hex_map->map_to_local(cell);

		for (int j = 1; j < shape_points.size(); j++) {
			grid_points.append(center + shape_points[j - 1]);
			grid_points.append(center + shape_points[j]);
		}
	}

	// TypedArray<Vector3i> cells = hex_map->local_region_to_map(
	// 		Vector3i(-GRID_RADIUS * Math_SQRT3 * p_cell_size.x, 0,
	// 				-GRID_RADIUS * 1.625 * p_cell_size.x),
	// 		Vector3i(GRID_RADIUS * Math_SQRT3 * p_cell_size.x, 0,
	// 				GRID_RADIUS * 1.625 * p_cell_size.x));
	// for (int i = 0; i < cells.size(); i++) {
	// 	Vector3 center = hex_map->map_to_local(cells[i]);
	//
	// 	for (int j = 1; j < shape_points.size(); j++) {
	// 		grid_points.append(center + shape_points[j - 1]);
	// 		grid_points.append(center + shape_points[j]);
	// 	}
	// }

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->mesh_add_surface_from_arrays(
			grid_mesh, RenderingServer::PRIMITIVE_LINES, d);
	rs->mesh_surface_set_material(grid_mesh, 0, grid_mat->get_rid());
}

void GridManager::build_qrs_grid() {
	// XXX STILL WORKING HERE
	// XXX paused this to add radius & axis support to
	//     HexMap.get_cell_neighbors
	// XXX paused that to create HexMap::CellId
	//
	// The idea here is that I can do:
	//		HexMapCellId::ORIGIN.get_neighbors(GRID_RADIUS, active_axis)
	//
	// then all of the build grids can be composed of a cell shape drawn about
	// the center of the cell.

	Vector3 p_cell_size(hex_map->get_cell_size());

	// create the points that make up the top of a hex cell
	Vector<Vector3> shape_points;
	shape_points.append(Vector3(-0.5, 0.5, 0) * p_cell_size);
	shape_points.append(Vector3(-0.5, -0.5, 0) * p_cell_size);
	shape_points.append(Vector3(0.5, -0.5, 0) * p_cell_size);
	shape_points.append(Vector3(0.5, 0.5, 0) * p_cell_size);

	PackedVector3Array grid_points;
	TypedArray<Vector3i> cells = hex_map->local_region_to_map(
			Vector3i(-GRID_RADIUS * p_cell_size.x,
					-GRID_RADIUS * p_cell_size.y, 0),
			Vector3i(-GRID_RADIUS * p_cell_size.x,
					-GRID_RADIUS * p_cell_size.y, 0));
	for (int i = 0; i < cells.size(); i++) {
		Vector3 center = hex_map->map_to_local(cells[i]);

		for (int j = 1; j < shape_points.size(); j++) {
			grid_points.append(center + shape_points[j - 1]);
			grid_points.append(center + shape_points[j]);
		}
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->mesh_add_surface_from_arrays(
			grid_mesh, RenderingServer::PRIMITIVE_LINES, d);
	rs->mesh_surface_set_material(grid_mesh, 0, grid_mat->get_rid());
}

void GridManager::build_grid() {
	RenderingServer::get_singleton()->mesh_clear(grid_mesh);
	switch (axis) {
		case EditorControl::AXIS_X:
		case EditorControl::AXIS_Y:
			build_y_grid();
			break;
		case EditorControl::AXIS_Q:
		case EditorControl::AXIS_R:
		case EditorControl::AXIS_S:
			break;
	}
}

void GridManager::set_axis(EditorControl::EditAxis value) {
	axis = value;
	build_grid();
}

void GridManager::set_depth(int value) {
	depth = value;
	// update_grid()
}

void GridManager::hide() {
	RenderingServer::get_singleton()->instance_set_visible(
			grid_mesh_instance, false);
}

void GridManager::show() {
	RenderingServer::get_singleton()->instance_set_visible(
			grid_mesh_instance, true);
}

GridManager::GridManager(HexMap *hex_map) : hex_map(hex_map) {
	RenderingServer *rs = RenderingServer::get_singleton();

	grid_mat.instantiate();
	grid_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	grid_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	grid_mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	grid_mat->set_flag(
			StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	grid_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	grid_mat->set_albedo(Color(0.8, 0.5, 0.1));

	grid_mesh = rs->mesh_create();

	RID scenario = EditorInterface::get_singleton()
						   ->get_editor_viewport_3d()
						   ->get_tree()
						   ->get_root()
						   ->get_world_3d()
						   ->get_scenario();
	grid_mesh_instance = rs->instance_create2(grid_mesh, scenario);
	// 24 = Node3DEditorViewport::MISC_TOOL_LAYER
	rs->instance_set_layer_mask(grid_mesh_instance, 1 << 24);

	hide();
}

GridManager::~GridManager() {
	RenderingServer *rs = RenderingServer::get_singleton();
	rs->free_rid(grid_mesh);
	rs->free_rid(grid_mesh_instance);
}
