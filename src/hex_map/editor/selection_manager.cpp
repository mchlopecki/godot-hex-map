#include "selection_manager.h"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/sub_viewport.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

void SelectionManager::build_cell_mesh() {
	mesh_mat.instantiate();
	mesh_mat->set_albedo(Color(0.7, 0.7, 1.0, 0.2));
	mesh_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	mesh_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	mesh_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

	line_mat.instantiate();
	line_mat->set_albedo(Color(0.7, 0.7, 1.0, 0.8));
	// outer_mat->set_on_top_of_alpha(); ->
	line_mat->set_transparency(godot::BaseMaterial3D::TRANSPARENCY_DISABLED);
	line_mat->set_render_priority(
			RenderingServer::MATERIAL_RENDER_PRIORITY_MAX);
	line_mat->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);

	line_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	line_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	line_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

	Array mesh_array, lines_array;
	mesh_array.resize(RS::ARRAY_MAX);
	lines_array.resize(RS::ARRAY_MAX);

	/*
	 *               (0)             Y
	 *              /   \            |
	 *           (1)     (5)         o---X
	 *            |       |           \
	 *           (2)     (4)           Z
	 *            | \   / |
	 *            |  (3)  |
	 *            |   |   |
	 *            |  (6)  |
	 *            | / | \ |
	 *           (7)  |  (b)
	 *            |   |   |
	 *           (8)  |  (a)
	 *              \ | /
	 *               (9)
	 */
	mesh_array[RS::ARRAY_VERTEX] = lines_array[RS::ARRAY_VERTEX] =
			PackedVector3Array({
					Vector3(0.0, 0.5, -1.0), // 0
					Vector3(-SQRT3_2, 0.5, -0.5), // 1
					Vector3(-SQRT3_2, 0.5, 0.5), // 2
					Vector3(0.0, 0.5, 1.0), // 3
					Vector3(SQRT3_2, 0.5, 0.5), // 4
					Vector3(SQRT3_2, 0.5, -0.5), // 5
					Vector3(0.0, -0.5, -1.0), // 6
					Vector3(-SQRT3_2, -0.5, -0.5), // 7
					Vector3(-SQRT3_2, -0.5, 0.5), // 8
					Vector3(0.0, -0.5, 1.0), // 9
					Vector3(SQRT3_2, -0.5, 0.5), // 10 (0xa)
					Vector3(SQRT3_2, -0.5, -0.5), // 11 (0xb)
			});

	// clang-format off
	mesh_array[RS::ARRAY_INDEX] = PackedInt32Array({
		// top
		0, 5, 1,
		1, 5, 2,
		2, 5, 4,
		2, 4, 3,
		// bottom
		6, 7, 11,
		11, 7, 8,
		8, 10, 11,
		10, 8, 9,
		// east
		4,  5, 11,
		11, 10, 4,
		// northeast
		5, 0,  6,
		6, 11, 5,
		// northwest
		0, 1, 7,
		7, 6, 0,
		// west
		1, 2, 8,
		8, 7, 1,
		// southwest
		2, 3, 9,
		9, 8, 2,
		// southeast
		3, 4, 10,
		10, 9, 3,
	});
	// clang-format on

	lines_array[RS::ARRAY_INDEX] = PackedInt32Array({
			0, 1, 2, 3, 4, 5, // top
			11, 6, 7, 8, 9, 10, // bottom
			11, 5, 0, 6, // northeast face
			7, 1, 2, 8, // west face
			9, 3, 4, 10, // southeast face
	});

	RenderingServer *rs = RS::get_singleton();
	RID cell_mesh = rs->mesh_create();
	rs->mesh_add_surface_from_arrays(
			cell_mesh, RS::PRIMITIVE_TRIANGLES, mesh_array);
	rs->mesh_surface_set_material(cell_mesh, 0, mesh_mat->get_rid());

	// add lines around the cell
	rs->mesh_add_surface_from_arrays(
			cell_mesh, RS::PRIMITIVE_LINE_STRIP, lines_array);
	rs->mesh_surface_set_material(cell_mesh, 1, line_mat->get_rid());

	// create the multimesh for rendering the tile mesh in multiple locations.
	selection_multimesh = rs->multimesh_create();
	rs->multimesh_set_mesh(selection_multimesh, cell_mesh);

	RID scenario = EditorInterface::get_singleton()
						   ->get_editor_viewport_3d()
						   ->get_tree()
						   ->get_root()
						   ->get_world_3d()
						   ->get_scenario();
	selection_multimesh_instance =
			rs->instance_create2(selection_multimesh, scenario);
	rs->instance_set_layer_mask(selection_multimesh_instance,
			/* Node3DEditorViewport::MISC_TOOL_LAYER */ 1 << 24);
}

void SelectionManager::redraw_selection() {
	RenderingServer *rs = RS::get_singleton();

	// Scaling and translation for the center of the cell mesh.
	Vector3 cell_center = Vector3(
			hex_map->get_center_x() ? 0 : hex_map->get_cell_size().x / 2.0,
			hex_map->get_center_y() ? 0 : hex_map->get_cell_size().y / 2.0,
			hex_map->get_center_z() ? 0 : hex_map->get_cell_size().z / 2.0);
	Transform3D cell_transform =
			Transform3D()
					.scaled_local(hex_map->get_cell_size())
					.translated(cell_center);

	// Add the cells to our selection multimesh
	rs->multimesh_allocate_data(
			selection_multimesh, cells.size(), RS::MULTIMESH_TRANSFORM_3D);
	for (int i = 0; i < cells.size(); i++) {
		Vector3i cell = cells[i];
		rs->multimesh_instance_set_transform(selection_multimesh, i,
				cell_transform.translated(hex_map->map_to_local(cell)));
	}

	// transform the multimesh to match the HexMap transform
	rs->instance_set_transform(
			selection_multimesh_instance, hex_map->get_global_transform());
	// make sure the multimesh is visible
	RenderingServer::get_singleton()->instance_set_visible(
			selection_multimesh_instance, true);
}

void SelectionManager::clear() {
	cells.clear();
	redraw_selection();
	RenderingServer::get_singleton()->instance_set_visible(
			selection_multimesh_instance, false);
}

void SelectionManager::set_cell(HexMapCellId cell) {
	cells.append(cell);
	redraw_selection();
}

void SelectionManager::set_cells(Vector<HexMapCellId> other) {
	cells.append_array(other);
	redraw_selection();
}

HexMapCellId SelectionManager::get_center() {
	if (cells.is_empty()) {
		return HexMapCellId();
	}

	Vector3 center = cells[0].unit_center();
	Vector3 min = center, max = center;
	for (const HexMapCellId &cell : cells) {
		center = cell.unit_center();
		if (center.x < min.x) {
			min.x = center.x;
		} else if (center.x > max.x) {
			max.x = center.x;
		}
		if (center.y < min.y) {
			min.y = center.y;
		} else if (center.y > max.y) {
			max.y = center.y;
		}
		if (center.z < min.z) {
			min.z = center.z;
		} else if (center.z > max.z) {
			max.z = center.z;
		}
	}

	return HexMapCellId::from_unit_point((max + min) / 2);
}

SelectionManager::SelectionManager(HexMap *hex_map) : hex_map(hex_map) {
	build_cell_mesh();
}

SelectionManager::~SelectionManager() {
	RenderingServer *rs = RS::get_singleton();
	rs->free_rid(selection_multimesh_instance);
	rs->free_rid(selection_multimesh);
	rs->free_rid(cell_mesh);
}
