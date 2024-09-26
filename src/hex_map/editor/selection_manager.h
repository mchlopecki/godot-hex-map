#ifdef TOOLS_ENABLED

#pragma once
#include "../hex_map.h"
#include "../hex_map_cell_id.h"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/rid.hpp"

using namespace godot;

class SelectionManager {
    HexMap *hex_map;

    Ref<StandardMaterial3D> mesh_mat;
    Ref<StandardMaterial3D> line_mat;
    RID cell_mesh;
    RID selection_multimesh;
    RID selection_multimesh_instance;

    Vector<HexMapCellId> cells;

    void build_cell_mesh();

public:
    SelectionManager(HexMap *hex_map);
    ~SelectionManager();

    void show();
    void hide();
    bool is_visible();

    bool is_empty() const { return cells.is_empty(); };
    const Vector<HexMapCellId> get_cells() const { return cells; }

    // does not currently handle de-duplication
    void clear();
    void set_cell(HexMapCellId);
    void set_cells(Vector<HexMapCellId>);
    void redraw_selection(); // for cell size change

    // get cell id at the center of the selection
    HexMapCellId get_center();
};

#endif
