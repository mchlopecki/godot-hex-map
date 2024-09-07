#pragma once

#include "../hex_map.h"
#include "../hex_map_cell_id.h"
#include "../tile_orientation.h"
#include "editor_control.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/templates/list.hpp"
#include "godot_cpp/variant/plane.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/vector2.hpp"

using namespace godot;

class EditorCursor {
public:
	using CellId = HexMap::CellId;

	// representation of a tile state
	struct CursorCell {
		CellId cell_id;
		CellId cell_id_live;
		int tile = -1;
		TileOrientation orientation;
		RID mesh_instance;
	};

private:
	HexMap *hex_map;

	CellId pointer_cell;
	List<CursorCell> tiles;
	TileOrientation orientation;

	Vector3 last_update_origin;
	Vector3 last_update_normal;

	Plane edit_plane;
	EditorControl::EditAxis edit_axis = EditorControl::EditAxis::AXIS_Y;

	Ref<StandardMaterial3D> grid_mat;
	RID grid_mesh;
	RID grid_mesh_instance;
	Transform3D grid_mesh_transform;

	inline void transform_cell_mesh(
			RenderingServer *rs, MeshLibrary *mesh_library, CursorCell &cell);
	void transform_cell_mesh(CursorCell &cell);
	void transform_meshes();
	void free_tile_meshes();

	void build_y_grid();
	void build_r_grid();

protected:
public:
	EditorCursor(HexMap *hex_map);
	~EditorCursor();

	bool update(const Camera3D *camera, const Point2 &pointer, Vector3 *point);
	void update(bool force = false);
	// XXX need to rebuild grids & transform cells when grid size changes
	void show();
	void hide();
	void clear_tiles();
	void cell_size_changed();
	void set_axis(EditorControl::EditAxis axis);
	void set_depth(int);
	void set_tile(CellId cell, int tile,
			TileOrientation orientation = TileOrientation::Upright0);
	void set_pointer(CellId cell);
	void set_orientation(TileOrientation orientation);
	List<CursorCell> get_tiles();
	HexMapCellId get_cell() { return pointer_cell; };
};
