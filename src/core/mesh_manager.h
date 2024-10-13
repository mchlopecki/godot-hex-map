#pragma once

#include <cstdint>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_library.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include "cell_id.h"
#include "hash_map_key_iter.h"
#include "space.h"
#include "tile_orientation.h"

using namespace godot;

class HexMapMeshManager {
private:
    /// Type used as a key when looking up cells
    using CellId = HexMapCellId;
    using CellKey = CellId::Key;

    /// cell state
    struct Cell {
        /// mesh to show in the cell
        RID mesh;
        /// mesh transform including rotation
        Transform3D transform;
    };

    /// multimesh & multimesh instance pair
    struct MultiMesh {
        RID multimesh;
        RID instance;
    };

    /// missing mesh library mesh
    // would love to be able to share this across all instances of mesh
    // manager, but Ref<T> crashes in `godot::RefCounted::unreference()` when
    // this is used as a static.
    Ref<ArrayMesh> missing_mesh;

    /// map of cell keys to the visual for each cell
    HashMap<CellKey, Cell> cell_map;

    /// list of multimeshes
    Vector<MultiMesh> multimeshes;

    /// scenario for mesh instances
    RID scenario;

    /// godot object id all mesh instances belong to
    ObjectID object_id;

    /// hexagonal space to use for converting cell ids to points
    HexSpace space;

    void free_multimeshes();
    void build_multimeshes();

    /// used internally to generate the missing mesh mesh
    Ref<ArrayMesh> get_missing_mesh();

public:
    HexMapMeshManager(RID scenario = RID(), uint64_t object_id = 0) :
            scenario(scenario), object_id(object_id){};
    ~HexMapMeshManager();

    /// Set the scenario to add meshes to
    inline void set_scenario(RID value) { scenario = value; };

    /// Set the object id for all mesh instances; applies on next refresh()
    inline void set_object_id(ObjectID value) { object_id = value; };
    inline void set_object_id(uint64_t value) { object_id = (ObjectID)value; };

    /// Set the hex space parameters
    void set_space(const HexSpace &);

    /// Get the `HexSpace` for this MeshManager
    inline HexSpace &get_space() { return space; }

    /// Set the mesh and mesh instance transform for a given cell.  The
    /// transform is relative to the center of the cell.
    void set_cell(CellId cell_id,
            RID mesh,
            Transform3D mesh_transform = Transform3D());

    /// Set the mesh & rotation for a given cell from a MeshLibrary.
    ///
    /// This helper will get the mesh and mesh transform from the mesh library
    /// by the provided `index`.  If the `mesh_library` is null, or the `index`
    /// does not exist in the `mesh_library`, a placeholder mesh will be drawn
    /// in the cell.
    void set_cell(CellId,
            const Ref<MeshLibrary> &,
            int index,
            HexMapTileOrientation);

    /// clear the specified cell
    void clear_cell(CellId);

    /// get an iterator to go over the cell ids in the mesh
    const HashMap<CellKey, Cell> get_cells() const { return cell_map; };

    /// update the meshes that are displayed
    void refresh();

    /// clear all cells in the mesh
    void clear();

    /// set the visibility of any meshes present
    void set_visibility(bool visible);

    /// helper function to simplify needed calls when hexmap enters world
    void enter_world(RID scenario);

    /// helper function to simplify needed calls when hexmap exits world
    void exit_world();
};
