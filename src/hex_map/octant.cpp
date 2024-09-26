#include "octant.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_data_tool.hpp"
#include "godot_cpp/classes/mesh_library.hpp"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/shape3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/templates/pair.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "hex_map.h"
#include "profiling.h"
#include <cassert>

void HexMapOctant::rebuild() {
    auto profiler = profiling_begin("Octant::rebuild()");

    assert(hex_map.is_inside_tree() &&
            "should be only be called when HexMap is in SceneTree");
    RenderingServer *rs = RenderingServer::get_singleton();
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    Ref<MeshLibrary> &mesh_library = hex_map.mesh_library;

    ps->body_clear_shapes(physics_body);

    if (collision_debug_mesh.is_valid()) {
        rs->mesh_clear(collision_debug_mesh);
    }

    for (const auto &multimesh : multimeshes) {
        rs->free_rid(multimesh.multimesh_instance);
        rs->free_rid(multimesh.multimesh);
    }
    multimeshes.clear();

    // can't do anything without the MeshLibrary
    if (!mesh_library.is_valid()) {
        return;
    }

    // to create the multimesh for cells, we need the mesh RID and the
    // transform for each instance of that cell in the octant
    HashMap<RID, Vector<Transform3D>> mesh_cells;

    // to update the collision debugging mesh, we need the vertices for all of
    // the collision shapes
    PackedVector3Array debug_mesh_vertices;

    // iterate through the cells, save off RID & Transform pairs for multimesh
    // creation later, update the physics body, and if collision debugging is
    // enabled, update that mesh.
    for (const CellKey &cell_key : cells) {
        const HexMap::Cell *cell = hex_map.cell_map.getptr(cell_key);
        ERR_CONTINUE_MSG(cell == nullptr, "nonexistent HexMap cell in Octant");

        Ref<Mesh> mesh = mesh_library->get_item_mesh(cell->item);
        if (!mesh.is_valid()) {
            continue;
        }
        RID mesh_rid = mesh->get_rid();

        Vector<Transform3D> *mesh_transforms = mesh_cells.getptr(mesh_rid);
        if (!mesh_cells.has(mesh_rid)) {
            auto iter = mesh_cells.insert(mesh_rid, Vector<Transform3D>());
            mesh_transforms = &iter->value;
        }

        Transform3D cell_transform;
        cell_transform.basis = cell->get_basis();
        cell_transform.set_origin(hex_map.cell_id_to_local(cell_key));

        // save off the mesh transform for this cell; we'll use it when
        // creating the multimesh for this cell type later.
        mesh_transforms->push_back(cell_transform *
                mesh_library->get_item_mesh_transform(cell->item));

        // Update the static body for the octant if the mesh library has any
        // collision shapes for this cell type.  Note that the collision shape
        // has its own transform independent of the mesh transform.  Also
        // get_item_shapes() returns an array of Shape3D followed by
        // Transform3D for each shape.
        const Array shapes = mesh_library->get_item_shapes(cell->item);
        for (int i = 0; i < shapes.size(); i += 2) {
            Shape3D *shape = Object::cast_to<Shape3D>(shapes[i]);
            Transform3D shape_transform =
                    cell_transform * (Transform3D)shapes[i + 1];

            // add the shape to the physics body
            ps->body_add_shape(
                    physics_body, shape->get_rid(), shape_transform);

            // if we have a collision debugging mesh, add the shape vertices to
            // the vertex array.
            if (collision_debug_mesh.is_valid()) {
                const Ref<ArrayMesh> &debug_mesh = shape->get_debug_mesh();
                const Array arrays = debug_mesh->surface_get_arrays(0);
                assert(arrays.size() > Mesh::ARRAY_VERTEX &&
                        "arrays should include vertex arrays");
                const PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
                for (const Vector3 &v : vertices) {
                    debug_mesh_vertices.append(shape_transform.xform(v));
                }
            }
        }
    }

    // create a multimesh for each tile type present in the octant
    for (const auto &pair : mesh_cells) {
        // create the multimesh
        RID multimesh = rs->multimesh_create();
        rs->multimesh_set_mesh(multimesh, pair.key);
        rs->multimesh_allocate_data(
                multimesh, pair.value.size(), RS::MULTIMESH_TRANSFORM_3D);

        // copy all the transforms into it
        const Vector<Transform3D> &transforms = pair.value;
        for (int i = 0; i < transforms.size(); i++) {
            rs->multimesh_instance_set_transform(multimesh, i, transforms[i]);
        }

        // create an instance of the multimesh
        RID instance = rs->instance_create2(
                multimesh, hex_map.get_world_3d()->get_scenario());

        multimeshes.push_back(MultiMesh{ multimesh, instance });
    }

    // update the collision debugging mesh if one exists
    if (collision_debug_mesh.is_valid() && !debug_mesh_vertices.is_empty()) {
        profiling_emit("updating collision debug mesh");
        Array surface_arrays;
        surface_arrays.resize(RS::ARRAY_MAX);
        surface_arrays[RS::ARRAY_VERTEX] = debug_mesh_vertices;
        rs->mesh_add_surface_from_arrays(
                collision_debug_mesh, RS::PRIMITIVE_LINES, surface_arrays);
        rs->mesh_surface_set_material(collision_debug_mesh,
                0,
                hex_map.collision_debug_mat->get_rid());
    }

    // everything has been built using the local position within the HexMap,
    // now adjust everything for the global position of the HexMap.
    apply_global_transform();
}

void HexMapOctant::apply_global_transform() {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    RenderingServer *rs = RenderingServer::get_singleton();
    Transform3D global_transform = hex_map.get_global_transform();

    // move the physics body
    ps->body_set_state(physics_body,
            PhysicsServer3D::BODY_STATE_TRANSFORM,
            global_transform);

    // move the collision debugging mesh instance if present
    if (collision_debug_mesh_instance.is_valid()) {
        rs->instance_set_transform(
                collision_debug_mesh_instance, global_transform);
    }

    // move each of the multimeshes
    for (const auto &multimesh : multimeshes) {
        rs->instance_set_transform(
                multimesh.multimesh_instance, global_transform);
    }
}

void HexMapOctant::update_collision_properties() {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    ps->body_set_collision_layer(physics_body, hex_map.collision_layer);
    ps->body_set_collision_mask(physics_body, hex_map.collision_mask);
    ps->body_set_collision_layer(physics_body, hex_map.collision_layer);
}

void HexMapOctant::update_physics_params() {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    ps->body_set_param(physics_body,
            PhysicsServer3D::BODY_PARAM_FRICTION,
            hex_map.physics_body_friction);
    ps->body_set_param(physics_body,
            PhysicsServer3D::BODY_PARAM_BOUNCE,
            hex_map.physics_body_bounce);
}

void HexMapOctant::update_transform() {
    // only valid if the HexMap is inside a scene tree
    if (hex_map.is_inside_tree()) {
        apply_global_transform();
    }
}

void HexMapOctant::update_visibility() {
    ERR_FAIL_COND(!hex_map.is_inside_tree());
    RenderingServer *rs = RenderingServer::get_singleton();

    // XXX should hidden also disable the static shape?

    bool visible = hex_map.is_visible_in_tree();
    for (const auto &mm : multimeshes) {
        rs->instance_set_visible(mm.multimesh_instance, visible);
    }
    if (collision_debug_mesh_instance.is_valid()) {
        rs->instance_set_visible(collision_debug_mesh_instance, visible);
    }
}

void HexMapOctant::enter_world() {
    ERR_FAIL_COND(!hex_map.is_inside_tree());

    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    RenderingServer *rs = RenderingServer::get_singleton();
    SceneTree *st = hex_map.get_tree();

    ps->body_set_space(physics_body, hex_map.get_world_3d()->get_space());

    // if debugging collisions, create collision debug mesh and show it
    if (hex_map.collision_debug || st->is_debugging_collisions_hint()) {
        assert(!collision_debug_mesh.is_valid() && "mesh should not exist");
        assert(!collision_debug_mesh_instance.is_valid() &&
                "mesh instance should not exist");

        collision_debug_mesh = rs->mesh_create();
        collision_debug_mesh_instance = rs->instance_create2(
                collision_debug_mesh, hex_map.get_world_3d()->get_scenario());
    }

    rebuild();
}

void HexMapOctant::exit_world() {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    RenderingServer *rs = RenderingServer::get_singleton();

    ps->body_set_space(physics_body, RID());

    for (const MultiMesh &multimesh : multimeshes) {
        rs->free_rid(multimesh.multimesh_instance);
        rs->free_rid(multimesh.multimesh);
    }
    multimeshes.clear();

    rs->free_rid(collision_debug_mesh_instance);
    collision_debug_mesh_instance = RID();

    rs->free_rid(collision_debug_mesh);
    collision_debug_mesh = RID();
}

void HexMapOctant::update() {
    if (hex_map.is_inside_tree()) {
        rebuild();
    }
}

void HexMapOctant::add_cell(const CellKey cell_key) {
    cells.insert(cell_key);
    dirty = true;
}

void HexMapOctant::remove_cell(const CellKey cell_key) {
    cells.erase(cell_key);
    dirty = true;
}

HexMapOctant::HexMapOctant(HexMap &hex_map) : hex_map(hex_map) {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();

    physics_body = ps->body_create();
    ps->body_set_mode(physics_body, PhysicsServer3D::BODY_MODE_STATIC);
    ps->body_attach_object_instance_id(
            physics_body, hex_map.get_instance_id());
    update_collision_properties();
    update_physics_params();
}

HexMapOctant::~HexMapOctant() {
    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
    ps->free_rid(physics_body);
}
