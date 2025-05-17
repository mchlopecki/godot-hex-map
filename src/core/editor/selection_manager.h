#ifdef TOOLS_ENABLED
#pragma once

#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "core/cell_id.h"
#include "core/mesh_tool.h"
#include "core/space.h"

using namespace godot;

class SelectionManager {
public:
    SelectionManager(RID scenario);
    ~SelectionManager();

    void set_space(HexMapSpace space);

    void show();
    void hide();
    bool is_visible();

    Vector<HexMapCellId> get_cell_ids() const;
    Array get_cell_vecs() const;

    void clear();
    size_t size() const;
    bool is_empty() const;
    void add_cell(HexMapCellId);
    void set_cells(Vector<HexMapCellId>);
    void set_cells(Array);
    void redraw_selection(); // for cell size change

    /// get cell id that represents the center of the selection
    HexMapCellId get_center();

private:
    Ref<ArrayMesh> cell_mesh;
    Ref<StandardMaterial3D> mesh_mat;
    Ref<StandardMaterial3D> line_mat;
    HexMapMeshTool mesh_manager;

    void build_cell_mesh();
};

#endif
