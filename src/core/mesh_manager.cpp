#include "mesh_manager.h"
#include "../profiling.h"
#include "hash_map_key_iter.h"
#include "math.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

// XXX break this up and move the pieces someplace more appropriate, maybe
// a HexagonMesh?
Ref<ArrayMesh> HexMapMeshManager::get_missing_mesh() {
    if (missing_mesh.is_valid()) {
        return missing_mesh;
    }

    Ref<StandardMaterial3D> surface_mat;
    surface_mat.instantiate();
    surface_mat->set_albedo(Color(1.7, 0.1, 0.1, 0.7));
    surface_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
    surface_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
    surface_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

    Array surface;
    surface.resize(RenderingServer::ARRAY_MAX);

    /*
     *               (0)             Y
     *              /   \            |
     *           (1)     (5)         o---X
     *            |       |           \
     *           (2)     (4)           Z
     *            | \   / |
     *            |  (3)  |
     *            |   |   |
     *            |  (6)  |
     *            | / | \ |
     *           (7)  |  (b)
     *            |   |   |
     *           (8)  |  (a)
     *              \ | /
     *               (9)
     */
    surface[RenderingServer::ARRAY_VERTEX] = PackedVector3Array({
            Vector3(0.0, 0.5, -1.0), // 0
            Vector3(-Math_SQRT3_2, 0.5, -0.5), // 1
            Vector3(-Math_SQRT3_2, 0.5, 0.5), // 2
            Vector3(0.0, 0.5, 1.0), // 3
            Vector3(Math_SQRT3_2, 0.5, 0.5), // 4
            Vector3(Math_SQRT3_2, 0.5, -0.5), // 5
            Vector3(0.0, -0.5, -1.0), // 6
            Vector3(-Math_SQRT3_2, -0.5, -0.5), // 7
            Vector3(-Math_SQRT3_2, -0.5, 0.5), // 8
            Vector3(0.0, -0.5, 1.0), // 9
            Vector3(Math_SQRT3_2, -0.5, 0.5), // 10 (0xa)
            Vector3(Math_SQRT3_2, -0.5, -0.5), // 11 (0xb)
    });

    // clang-format off
	surface[RenderingServer::ARRAY_INDEX] = PackedInt32Array({
		// top
		0, 5, 1,
		1, 5, 2,
		2, 5, 4,
		2, 4, 3,
		// bottom
		6, 7, 11,
		11, 7, 8,
		8, 10, 11,
		10, 8, 9,
		// east
		4,  5, 11,
		11, 10, 4,
		// northeast
		5, 0,  6,
		6, 11, 5,
		// northwest
		0, 1, 7,
		7, 6, 0,
		// west
		1, 2, 8,
		8, 7, 1,
		// southwest
		2, 3, 9,
		9, 8, 2,
		// southeast
		3, 4, 10,
		10, 9, 3,
	});
    // clang-format on

    missing_mesh.instantiate();
    missing_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface);
    missing_mesh->surface_set_material(0, surface_mat);

    return missing_mesh;
}

void HexMapMeshManager::set_space(const HexSpace &value) { space = value; }

void HexMapMeshManager::free_multimeshes() {
    RenderingServer *rs = RenderingServer::get_singleton();

    for (const auto &multimesh : multimeshes) {
        rs->free_rid(multimesh.instance);
        rs->free_rid(multimesh.multimesh);
    }
    multimeshes.clear();
}

void HexMapMeshManager::build_multimeshes() {
    ERR_FAIL_COND_MSG(!scenario.is_valid(),
            "HexMapMeshManager instances does not have a valid scenario");

    auto profiler = profiling_begin("HexMapMeshManager::build_multimeshes()");

    // to create the multimesh for cells, we need the mesh RID and the
    // transform for each instance of that cell in the octant
    HashMap<RID, Vector<Transform3D>> mesh_cells;

    // iterate through the cells, save off RID & Transform pairs for multimesh
    // creation later.
    for (const auto &iter : cell_map) {
        CellId cell_id(iter.key);
        const Cell &cell = iter.value;

        Vector<Transform3D> *mesh_transforms = mesh_cells.getptr(cell.mesh);
        if (mesh_transforms == nullptr) {
            auto iter = mesh_cells.insert(cell.mesh, Vector<Transform3D>());
            mesh_transforms = &iter->value;
        }

        mesh_transforms->push_back(
                space.get_mesh_transform(cell_id) * cell.transform);
    }

    RenderingServer *rs = RenderingServer::get_singleton();

    // create a multimesh for each tile type present in the octant
    for (const auto &pair : mesh_cells) {
        // create the multimesh
        RID multimesh = rs->multimesh_create();
        rs->multimesh_set_mesh(multimesh, pair.key);
        rs->multimesh_allocate_data(multimesh,
                pair.value.size(),
                RenderingServer::MULTIMESH_TRANSFORM_3D);

        // copy all the transforms into it
        const Vector<Transform3D> &transforms = pair.value;
        for (int i = 0; i < transforms.size(); i++) {
            rs->multimesh_instance_set_transform(multimesh, i, transforms[i]);
        }

        // create an instance of the multimesh
        RID instance = rs->instance_create2(multimesh, scenario);
        rs->instance_attach_object_instance_id(instance, object_id);

        multimeshes.push_back(MultiMesh{ multimesh, instance });
    }
}

// XXX maybe implement a performance oriented update that preserves multimesh
// instances when possible.  Should be possible unless the cell_map changes.

void HexMapMeshManager::set_cell(CellId cell_id,
        RID mesh,
        Transform3D mesh_transform) {
    ERR_FAIL_COND_MSG(
            !mesh.is_valid(), "invalid mesh provided for cell " + cell_id);

    cell_map.insert(
            cell_id, Cell{ .mesh = mesh, .transform = mesh_transform });
}

void HexMapMeshManager::set_cell(CellId cell_id,
        const Ref<MeshLibrary> &mesh_library,
        int index,
        HexMapTileOrientation orientation) {
    Ref<Mesh> mesh;
    Transform3D mesh_transform;

    // try to get the mesh from the mesh library
    if (mesh_library.is_valid()) {
        mesh = mesh_library->get_item_mesh(index);
        mesh_transform = mesh_library->get_item_mesh_transform(index);
    }
    // if we weren't able to find the mesh, use a placeholder mesh to denote
    // invalid cell.
    if (!mesh.is_valid()) {
        mesh = HexMapMeshManager::get_missing_mesh();
        mesh_transform = Transform3D(Basis::from_scale(space.get_cell_scale()),
                -space.get_mesh_offset());
    }

    Transform3D cell_transform(orientation);
    set_cell(cell_id, mesh->get_rid(), cell_transform * mesh_transform);
};

void HexMapMeshManager::clear_cell(CellId key) { cell_map.erase(key); }

void HexMapMeshManager::refresh() {
    free_multimeshes();
    build_multimeshes();
}

void HexMapMeshManager::clear() {
    free_multimeshes();
    cell_map.clear();
}

void HexMapMeshManager::set_visibility(bool visible) {
    if (!scenario.is_valid()) {
        return;
    }
    RenderingServer *rs = RenderingServer::get_singleton();
    for (const MultiMesh &mm : multimeshes) {
        rs->instance_set_visible(mm.instance, visible);
    }
}

void HexMapMeshManager::enter_world(RID scenario) {
    set_scenario(scenario);
    refresh();
}

void HexMapMeshManager::exit_world() {
    set_scenario(RID());
    free_multimeshes();
}

HexMapMeshManager::~HexMapMeshManager() { free_multimeshes(); }
