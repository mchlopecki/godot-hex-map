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
#include "space.h"

using namespace godot;

class HexMapOctant;

class HexMapBase : public Node3D {
    GDCLASS(HexMapBase, Node3D);

protected:
    HexSpace space;

    static void _bind_methods();

public:
    // various cell size getters/setters
    void set_cell_height(real_t p_height);
    real_t get_cell_height() const;
    void set_cell_radius(real_t p_radius);
    real_t get_cell_radius() const;
    void set_center_y(bool value);
    bool get_center_y() const;
    inline const HexSpace &get_space() { return space; }

    /// Return the cell scale factor for cells in this hexmap.
    ///
    /// A scale factor of `Vector3(1,1,1)` is returned for `cell_radius = 1.0`,
    /// `cell_height = 1.0`.
    Vector3 get_cell_scale() const;

    /// called when the cell scale changes
    virtual bool cell_scale_changed();

    /// given a cell id, return the local position of the cell
    Vector3 get_cell_center(const HexMapCellId &) const;

    /// given a local position, return the cell id that contains that point
    HexMapCellId get_cell_id(Vector3) const;
};
