#pragma once

#include "../hex_map.h"
#include "editor_control.h"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"

using namespace godot;

class GridManager {
private:
	HexMap *hex_map = nullptr;

	Ref<StandardMaterial3D> grid_mat;
	RID grid_mesh;
	RID grid_mesh_instance;

	EditorControl::EditAxis axis = EditorControl::EditAxis::AXIS_Y;
	int depth = 0;

	void build_grid();
	void build_x_grid();
	void build_y_grid();
	void build_qrs_grid();

public:
	GridManager(HexMap *hex_map);
	~GridManager();

	void hide();
	void show();

	void set_axis(EditorControl::EditAxis);
	void set_depth(int);
};
