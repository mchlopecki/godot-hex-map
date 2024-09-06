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
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/vector3.hpp"

#define GRID_RADIUS 40u

// create a mesh and draw a grid of hexagonal cells on it
void GridManager::build_y_grid() {
	Vector3 p_cell_size(hex_map->get_cell_size());

	// create the points that make up the top of a hex cell
	Vector<Vector3> shape_points;
	shape_points.append(Vector3(0.0, 0, -1.0) );
	shape_points.append(Vector3(-SQRT3_2, 0, -0.5) );
	shape_points.append(Vector3(-SQRT3_2, 0, 0.5) );
	shape_points.append(Vector3(0.0, 0, 1.0) );
	shape_points.append(Vector3(SQRT3_2, 0, 0.5) );
	shape_points.append(Vector3(SQRT3_2, 0, -0.5) );
	shape_points.append(Vector3(0.0, 0, -1.0) );

	// pick a point on the middle of an edge to find the closest edge of the
	// cells
	float max = hex_map->map_to_local(HexMapCellId(GRID_RADIUS,
											  -(int)GRID_RADIUS / 2, 0))
						.length_squared();
	PackedVector3Array grid_points;
	PackedColorArray grid_colors;
	for (const auto cell : HexMapCellId::Origin.get_neighbors(
				 GRID_RADIUS, HexMap::Planes::QRS)) {
		Vector3 center = cell.unit_center();
		float transparency =
				Math::pow(MAX(0, (max - center.length_squared()) / max), 2);

		for (int j = 1; j < shape_points.size(); j++) {
			grid_points.append(center + shape_points[j - 1]);
			grid_points.append(center + shape_points[j]);
			grid_colors.push_back(Color(1, 1, 1, transparency));
			grid_colors.push_back(Color(1, 1, 1, transparency));
		}
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;
	d[RS::ARRAY_COLOR] = grid_colors;

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->mesh_add_surface_from_arrays(
			grid_mesh, RenderingServer::PRIMITIVE_LINES, d);
	rs->mesh_surface_set_material(grid_mesh, 0, grid_mat->get_rid());
}

void GridManager::build_r_grid() {
	Vector3 p_cell_size(hex_map->get_cell_size());
	UtilityFunctions::print("cell size " + p_cell_size);

	// XXX need to adjust grid for center y
	// XXX works for R axis, but shape_points are wrong for Q/S, maybe add
	// rotation for those?  Just generate for R, then rotate +/- 60 degrees.
	// XXX store Transform3D instead of center cell then update origin when
	// pointer moves; allows us to preserve rotation for axis

	// create the points that make up the top of a hex cell
	Array shape_points;
	shape_points.append(Vector3(-SQRT3_2, 1.0, 0) );
	shape_points.append(Vector3(-SQRT3_2, 0, 0) );
	shape_points.append(Vector3(SQRT3_2, 0, 0) );
	shape_points.append(Vector3(SQRT3_2, 1.0, 0) );
	shape_points.append(Vector3(-SQRT3_2, 1.0, 0) );
	UtilityFunctions::print("cell points ", shape_points);

	float max = hex_map->map_to_local(HexMapCellId(0, 0, GRID_RADIUS))
						.length_squared();
	PackedVector3Array grid_points;
	PackedColorArray grid_colors;
	for (const auto cell : HexMapCellId::Origin.get_neighbors(
				 GRID_RADIUS, HexMap::Planes::YQS)) {
		Vector3 center = cell.unit_center();
		float transparency =
				Math::pow(MAX(0, (max - center.length_squared()) / max), 2);

		for (int j = 1; j < shape_points.size(); j++) {
			grid_points.append(center + shape_points[j - 1]);
			grid_points.append(center + shape_points[j]);
			grid_colors.push_back(Color(1, 1, 1, transparency));
			grid_colors.push_back(Color(1, 1, 1, transparency));
		}
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;
	d[RS::ARRAY_COLOR] = grid_colors;

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->mesh_add_surface_from_arrays(
			grid_mesh, RenderingServer::PRIMITIVE_LINES, d);
	rs->mesh_surface_set_material(grid_mesh, 0, grid_mat->get_rid());
}

void GridManager::build_grid() {
	// reset the mesh transform because some axis will rotate it
	Vector3 center = grid_mesh_transform.origin;
	grid_mesh_transform = Transform3D();
	grid_mesh_transform.set_origin(center);

	// clear the mesh
	RenderingServer *rs = RenderingServer::get_singleton();
	rs->mesh_clear(grid_mesh);

	switch (axis) {
		case EditorControl::AXIS_X:
		case EditorControl::AXIS_Y:
			build_y_grid();
			break;
		case EditorControl::AXIS_Q:
			build_r_grid();
			grid_mesh_transform.rotate(Vector3(0, 1, 0), -Math_PI / 3.0);
			break;
		case EditorControl::AXIS_R:
			build_r_grid();
			break;
		case EditorControl::AXIS_S:
			build_r_grid();
			grid_mesh_transform.rotate(Vector3(0, 1, 0), Math_PI / 3.0);
			break;
	}

	grid_mesh_transform.scale(hex_map->get_cell_size());
	RenderingServer::get_singleton()->instance_set_transform(
			grid_mesh_instance, grid_mesh_transform);
}

void GridManager::set_axis(EditorControl::EditAxis value) {
	axis = value;
	build_grid();
}

void GridManager::set_depth(int value) {
	depth = value;
	// update_grid()
}

void GridManager::set_center(const HexMapCellId center) {
	this->center = center;
	Vector3 point = hex_map->map_to_local(center);
	grid_mesh_transform.set_origin(point);
	RenderingServer::get_singleton()->instance_set_transform(
			grid_mesh_instance, grid_mesh_transform);
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
