#pragma once

#include "../hex_map.h"
#include "../tile_orientation.h"
#include "godot_cpp/templates/list.hpp"
#include "godot_cpp/variant/rid.hpp"

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
	List<CursorCell> tiles;
	CellId pointer_cell;
	TileOrientation orientation;

	inline void transform_cell_mesh(RenderingServer *rs,
			MeshLibrary *mesh_library,
			CursorCell &cell);
	void transform_cell_mesh(CursorCell &cell);
	void transform_cell_meshes();
	void free_meshes();

protected:
public:
	EditorCursor(HexMap *hex_map);
	~EditorCursor();

	void show();
	void hide();
	void clear_tiles();
	void redraw();
	void set_tile(CellId cell,
			int tile,
			TileOrientation orientation = TileOrientation::Upright0);
	void set_pointer(CellId cell);
	void set_orientation(TileOrientation orientation);
	List<CursorCell> get_tiles();
};
