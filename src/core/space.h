#pragma once

#include "godot_cpp/variant/packed_vector3_array.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "cell_id.h"

using namespace godot;

/// Helper class for mapping `HexMapCellId` to `Vector3` coordinates
class HexMapSpace {
    /// cell scale; scale from radius 1, height 1 cell size
    Vector3 cell_scale = Vector3(1, 1, 1);

    /// global transform of the meshes
    Transform3D global_transform;

public:
    /// Set the cell scaling for the hex grid.  Scale modifies the unit hex
    /// cell with `height = 1`, `radius = 1`.  This will update any
    /// `mesh_offset` that was previously set.
    void set_cell_scale(Vector3);

    /// Get the cell scale
    inline Vector3 get_cell_scale() const { return cell_scale; }

    void set_cell_height(real_t);
    inline real_t get_cell_height() const { return cell_scale.y; }

    void set_cell_radius(real_t);
    inline real_t get_cell_radius() const { return cell_scale.x; }

    /// Set the global transform of the meshes
    void set_transform(Transform3D);

    /// Get the global transform of the `HexSpace`
    inline Transform3D get_transform() const { return global_transform; }

    /// Get the center point for the given `cell` in local space
    inline Vector3 get_cell_center(const HexMapCellId &cell) const {
        return cell.unit_center() * cell_scale;
    }

    /// Get the center point for the given `cell` in local space
    inline Vector3 get_cell_center_global(const HexMapCellId &cell) const {
        return global_transform.xform(cell.unit_center() * cell_scale);
    }

    /// Get the `HexMapCellId` for a point in local space
    inline HexMapCellId get_cell_id(const Vector3 &local_pos) const {
        return HexMapCellId::from_unit_point(local_pos / cell_scale);
    }

    /// Get the `HexMapCellId` for a point in global space
    inline HexMapCellId get_cell_id_global(const Vector3 &global_pos) const {
        Vector3 local = global_transform.inverse().xform(global_pos);
        return get_cell_id(local);
    }

    inline bool operator!=(const HexMapSpace &other) const {
        return cell_scale != other.cell_scale ||
                global_transform != other.global_transform;
    }

    /// generate a PackedVector3Array for the vertices of a hex-shaped cell
    PackedVector3Array get_cell_vertices(
            Vector3 scale = Vector3(1, 1, 1)) const;

    /// generate an ArrayMesh that encompases the entire cell
    ///
    /// surface 0 contains the triangles, surface 1 contains the lines
    Ref<ArrayMesh> build_cell_mesh() const;

    /// return the `HexMapCellId` of every cell within a quad in local space
    ///
    /// @param a quad vertex
    /// @param b quad vertex
    /// @param c quad vertex
    /// @param d quad vertex
    /// @param padding percent to reduce the vertices of each cell to make it
    /// easier to select a cell withoug selecting the neighboring cell
    Vector<HexMapCellId> get_cell_ids_in_local_quad(Vector3 a,
            Vector3 b,
            Vector3 c,
            Vector3 d,
            float padding = 0.5) const;
};

// create a Ref<> type for HexMapSpace
namespace hex_bind {

class HexMapSpace : public RefCounted {
    GDCLASS(HexMapSpace, RefCounted)

public:
    HexMapSpace(const ::HexMapSpace &space) : inner(space) {};
    HexMapSpace() {};

    real_t get_cell_height() const;
    void set_cell_height(real_t);
    real_t get_cell_radius() const;
    void set_cell_radius(real_t);
    Vector3 get_cell_scale() const;
    void set_cell_scale(Vector3);

    Vector3 get_cell_center(const Ref<hex_bind::HexMapCellId> &) const;
    Ref<hex_bind::HexMapCellId> get_cell_id(Vector3) const;

    ::HexMapSpace inner;

protected:
    static void _bind_methods();
};

} // namespace hex_bind
