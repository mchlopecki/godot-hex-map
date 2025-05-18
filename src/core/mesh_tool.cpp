#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

#include "../profiling.h"
#include "mesh_tool.h"

void HexMapMeshTool::set_space(const HexMapSpace &value) { space = value; }

void HexMapMeshTool::set_mesh_origin(Vector3 value) { mesh_origin = value; }

void HexMapMeshTool::free_multimeshes() {
    RenderingServer *rs = RenderingServer::get_singleton();

    for (const auto &multimesh : multimeshes) {
        rs->free_rid(multimesh.instance);
        rs->free_rid(multimesh.multimesh);
    }
    multimeshes.clear();
}

// XXX maybe implement a performance oriented update that preserves multimesh
// instances when possible.  Should be possible unless the cell_map changes.
void HexMapMeshTool::build_multimeshes() {
    ERR_FAIL_COND_MSG(!scenario.is_valid(),
            "HexMapMeshManager instances does not have a valid scenario");

    auto profiler = profiling_begin("HexMapMeshManager::build_multimeshes()");

    // to create the multimesh for cells, we need the mesh RID and the
    // transform for each instance of that cell in the octant
    HashMap<RID, Vector<Transform3D>> mesh_cells;

    // calculate the mesh origin offset for each cell; this allows us to put
    // the origin at the bottom or top of the cell, instead of the center.
    Vector3 mesh_origin_offset = mesh_origin * space.get_cell_scale();

    // iterate through the cells, save off RID & Transform pairs for multimesh
    // creation later.
    for (const auto &iter : cell_map) {
        HexMapCellId cell_id(iter.key);
        const Cell &cell = iter.value;

        // skip hidden cells
        if (!cell.visible) {
            continue;
        }

        Vector<Transform3D> *mesh_transforms = mesh_cells.getptr(cell.mesh);
        if (mesh_transforms == nullptr) {
            auto iter = mesh_cells.insert(cell.mesh, Vector<Transform3D>());
            mesh_transforms = &iter->value;
        }

        Vector3 cell_center =
                space.get_cell_center_global(cell_id) + mesh_origin_offset;
        mesh_transforms->push_back(
                Transform3D(Basis(), cell_center) * cell.transform);
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
        rs->instance_set_visible(instance, visible);

        multimeshes.push_back(MultiMesh{ multimesh, instance });
    }
}

void HexMapMeshTool::set_cell(HexMapCellId cell_id,
        RID mesh,
        Transform3D mesh_transform) {
    ERR_FAIL_COND_MSG(
            !mesh.is_valid(), "invalid mesh provided for cell " + cell_id);

    cell_map.insert(
            cell_id, Cell{ .mesh = mesh, .transform = mesh_transform });
}

void HexMapMeshTool::clear_cell(HexMapCellId key) { cell_map.erase(key); }

void HexMapMeshTool::set_cell_visibility(HexMapCellId cell_id, bool visible) {
    Cell *cell = cell_map.getptr(cell_id);
    if (cell != nullptr) {
        cell->visible = visible;
    }
}

void HexMapMeshTool::set_all_cells_visible() {
    for (auto &iter : cell_map) {
        iter.value.visible = true;
    }
}

void HexMapMeshTool::refresh() {
    free_multimeshes();
    build_multimeshes();
}

void HexMapMeshTool::clear() {
    free_multimeshes();
    cell_map.clear();
}

bool HexMapMeshTool::get_visible() const { return visible; }

void HexMapMeshTool::set_visible(bool value) {
    visible = value;
    RenderingServer *rs = RenderingServer::get_singleton();
    for (const MultiMesh &mm : multimeshes) {
        rs->instance_set_visible(mm.instance, visible);
    }
}

void HexMapMeshTool::enter_world(RID scenario) {
    set_scenario(scenario);
    refresh();
}

void HexMapMeshTool::exit_world() {
    set_scenario(RID());
    free_multimeshes();
}

HexMapMeshTool::~HexMapMeshTool() { free_multimeshes(); }
