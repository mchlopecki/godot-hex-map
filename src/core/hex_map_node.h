#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_library.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_mesh_source_geometry_data3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/physics_material.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3i.hpp>

#include "cell_id.h"
#include "core/tile_orientation.h"
#include "space.h"

using namespace godot;

class HexMapNode : public Node3D {
    GDCLASS(HexMapNode, Node3D);

protected:
    HexMapSpace space;

    static void _bind_methods();
    void _notification(int p_what);

public:
    /// value used to denote that a cell has no value, so does not exist in the
    /// map.
    constexpr static int CELL_VALUE_NONE = -1;

    /// common contents for any cell in a hex node
    struct CellInfo {
        int value = CELL_VALUE_NONE;
        HexMapTileOrientation orientation;
    };

    /// This enum describes how cell data is organized in the Array returned
    /// by get_cells(), and expected by set_cells().  This is also the format
    /// used in the cells_changed signal.
    enum {
        /// cell ID as a Vector3i; we use this type to optimize packing as a
        /// Variant
        CELL_ARRAY_INDEX_VEC = 0,

        /// cell value
        CELL_ARRAY_INDEX_VALUE,

        /// cell orientation if present
        CELL_ARRAY_INDEX_ORIENTATION,

        /// Number of elements in the Array represent a single cell
        // must always be the last element in this enum
        CELL_ARRAY_WIDTH,
    };

    // various cell size getters/setters
    void set_cell_height(real_t p_height);
    real_t get_cell_height() const;
    void set_cell_radius(real_t p_radius);
    real_t get_cell_radius() const;

    /// set the HexMapSpace
    void set_space(const HexMapSpace &space);
    void set_space(const Ref<hex_bind::HexMapSpace> &);

    /// get the HexMapSpace
    inline const HexMapSpace &get_space() { return space; }
    Ref<hex_bind::HexMapSpace> _get_space();

    /// called when the cell scale changes
    virtual bool on_hex_space_changed();

    /// given a cell id, return the local position of the cell
    Vector3 get_cell_center(const HexMapCellId &) const;
    Vector3 get_cell_center(const Ref<hex_bind::HexMapCellId>) const;

    /// given a local position, return the cell id that contains that point
    HexMapCellId get_cell_id(Vector3) const;
    Ref<hex_bind::HexMapCellId> _get_cell_id(Vector3) const;

    /// set a single cell
    virtual void set_cell(const HexMapCellId &,
            int tile,
            HexMapTileOrientation orientation = 0) = 0;

    /// set a single cell
    void set_cell(const Ref<hex_bind::HexMapCellId> cell_id,
            int p_item,
            int p_orientation = 0);

    /// get the info for a single cell
    virtual CellInfo get_cell(const HexMapCellId &) const = 0;

    /// gdscript wrapper for get_cell
    Dictionary _get_cell(const Ref<hex_bind::HexMapCellId> &) const;

    /// set multiple cells
    ///
    /// `cells` is a flat Array containing three members for each cell:
    /// - `Vector3i` representing the cell id
    /// - int `value`
    /// - int `orientation`
    void set_cells(const Array p_cells);

    /// get tile & orientation for a subset of cells
    ///
    /// @param p_cells Array of Vector3i cell ids to look up
    /// @returns flat Array of [Vector3i, int, int] for each cell id requested.
    ///          The results will be in the same order as requested.
    Array get_cells(const Array p_cells);

    /// get the list of CellIds as Vector3i occupied in this node
    virtual Array get_cell_vecs() const = 0;

    /// return an Array of Vector3i cell ids for those cells with the supplied
    /// value
    ///
    /// @param value cell value
    /// @return Array array of Vector3i encoded cell IDs
    virtual Array find_cell_vecs_by_value(int value) const = 0;

    /// set the visibility of a cell
    ///
    /// This is used by HexMapEditorPlugin to show/hide cells that overlap the
    /// editor cursor.  This information should not be saved to the node.
    virtual void set_cell_visibility(const HexMapCellId &,
            bool visibility) = 0;

    /// set the visibility of a number of cells
    ///
    /// @param cells flattened Array of [Vector3, bool] where bool is the
    /// visibility state of the cell with id Vector3.
    void set_cells_visibility(const Array cells);

    /// return the `HexMapCellId` of every cell within a quad in local space
    /// @see HexSpace.get_cell_ids_in_local_quad()
    Array get_cell_ids_in_local_quad(Vector3 a,
            Vector3 b,
            Vector3 c,
            Vector3 d,
            float padding = 0.5) const;
};
