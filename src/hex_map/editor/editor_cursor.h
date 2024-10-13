#ifdef TOOLS_ENABLED
#pragma once

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "core/cell_id.h"
#include "core/library_mesh_tool.h"
#include "core/tile_orientation.h"
#include "editor_control.h"

using namespace godot;

class EditorCursor {
public:
    using CellId = HexMapCellId;
    using CellKey = HexMapCellId::Key;
    using TileOrientation = HexMapTileOrientation;

    //
    struct CellState {
        CellId cell_id;
        int index = -1;
        TileOrientation orientation;
    };

    EditorCursor(RID scenario);
    ~EditorCursor();

    void set_space(HexSpace);
    void set_mesh_library(Ref<MeshLibrary> &);
    void set_orientation(TileOrientation orientation);
    void set_axis(EditorControl::EditAxis axis);
    inline EditorControl::EditAxis get_axis() { return edit_axis; };

    /// set the depth of the edit plane
    void set_depth(int);

    bool update(const Camera3D *camera, const Point2 &pointer, Vector3 *point);
    void update(bool force = false);

    /// set the visibility of the tiles
    ///
    /// this does not apply to the visibility of the hex grid
    void set_visibility(bool visible);

    /// set a cell; takes effect on next `update()` call
    void set_tile(CellId cell,
            int tile,
            TileOrientation orientation = TileOrientation::Upright0);

    /// clear all cells; takes effect on next `update()` call
    void clear_tiles();

    inline bool is_empty() { return mesh_tool.get_cells().is_empty(); };
    inline size_t size() { return mesh_tool.get_cells().size(); };

    /// get the cell map for the cursor, adjusted for cursor transforms
    Vector<CellState> get_cells() const;

    /// Get the state of the only cell in the cursor.
    ///
    /// This function will assert if the number of cells set in the cursor does
    /// not equal 1.
    CellState get_only_cell_state() const;

    /// get the cell the cursor is occupying
    inline HexMapCellId get_cell() { return pointer_cell; };

    /// get the coordinates of the cursor on the edit plane
    inline Vector3 get_pos() { return pointer_pos; };

private:
    /// Mesh tool for drawing meshes
    HexMapLibraryMeshTool mesh_tool;

    EditorControl::EditAxis edit_axis = EditorControl::EditAxis::AXIS_Y;
    TileOrientation orientation;

    /// Copy of parent HexMap's HexSpace
    ///
    /// We keep this independent of the space in `mesh_tool` because that space
    /// includes our cursor rotation & translation.
    HexSpace parent_space;

    HexMapMeshTool mesh_manager;
    Vector3 pointer_pos;
    CellId pointer_cell;

    Vector3 last_update_origin;
    Vector3 last_update_normal;

    Plane edit_plane;

    Ref<StandardMaterial3D> grid_mat;
    RID grid_mesh;
    RID grid_mesh_instance;
    Transform3D grid_mesh_transform;

    /// Update the grid & tile meshes, along with `cursor_transform()`
    void transform_meshes();

    void build_y_grid();
    void build_r_grid();
};
#endif
