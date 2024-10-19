#ifdef TOOLS_ENABLED

#pragma once
#include "../hex_map.h"

#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>

using namespace godot;

class SelectionManager {
public:
    SelectionManager(RID scenario);
    ~SelectionManager();

    void set_space(HexSpace space);

    void show();
    void hide();
    bool is_visible();

    inline bool is_empty() const {
        return mesh_manager.get_cells().is_empty();
    };
    Vector<HexMap::CellId> get_cells() const;
    Array get_cells_v() const;

    void clear();
    void add_cell(HexMap::CellId);
    void set_cells(Vector<HexMap::CellId>);
    void set_cells(Array);
    void redraw_selection(); // for cell size change

    /// get cell id that represents the center of the selection
    HexMapCellId get_center();

private:
    Ref<StandardMaterial3D> mesh_mat;
    Ref<StandardMaterial3D> line_mat;
    RID cell_mesh;
    HexMapMeshTool mesh_manager;

    void build_cell_mesh();
};

#endif
