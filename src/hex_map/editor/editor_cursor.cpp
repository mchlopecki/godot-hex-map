#include "godot_cpp/core/error_macros.hpp"
#ifdef TOOLS_ENABLED

#include "../hex_map_cell_id.h"
#include "../hex_map_iter_axial.h"
#include "editor_cursor.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_plugin.hpp"
#include "godot_cpp/classes/mesh_library.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/sub_viewport.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include <cassert>

#define GRID_RADIUS 40u

void EditorCursor::free_tile_meshes() {
    RenderingServer *rs = RS::get_singleton();
    for (const CursorCell &cell : tiles) {
        if (cell.mesh_instance.is_valid()) {
            rs->free_rid(cell.mesh_instance);
        }
    }
}

void EditorCursor::clear_tiles() {
    free_tile_meshes();
    tiles.clear();
    orientation = TileOrientation::Upright0;
}

void EditorCursor::set_tile(CellId cell,
        int tile,
        TileOrientation orientation) {
    RenderingServer *rs = RS::get_singleton();

    // remove any duplicate cell in the existing cursor
    for (auto it = tiles.front(); it != nullptr; it = it->next()) {
        if (it->get().cell_id != cell) {
            continue;
        }
        rs->free_rid(it->get().mesh_instance);
        tiles.erase(it);
        break;
    }

    if (tile == -1) {
        return;
    }

    CursorCell cc = {
        .cell_id = cell, .tile = tile, .orientation = orientation
    };
    Ref<MeshLibrary> mesh_library = hex_map->get_mesh_library();
    RID scenario = hex_map->get_window()->get_world_3d()->get_scenario();
    RID mesh = mesh_library->get_item_mesh(tile)->get_rid();
    cc.mesh_instance = rs->instance_create2(mesh, scenario);
    tiles.push_back(cc);

    // XXX need to figure out a way to draw cursor cells over hexmap cells
}

List<EditorCursor::CursorCell> EditorCursor::get_tiles() {
    List<CursorCell> out;

    for (const CursorCell &cell : tiles) {
        CursorCell cc = {
            .cell_id = cell.cell_id + pointer_cell,
            .cell_id_live = cell.cell_id_live,
            .tile = cell.tile,
            .orientation = cell.orientation + orientation,
        };
        out.push_back(cc);
    }

    return out;
}

EditorCursor::CursorCell EditorCursor::get_first_tile() {
    List<CursorCell> out;
    if (tiles.is_empty()) {
        return CursorCell{
            .tile = -1,
        };
    }

    CursorCell &cell = tiles[0];
    return CursorCell{
        .cell_id = cell.cell_id + pointer_cell,
        .cell_id_live = cell.cell_id_live,
        .tile = cell.tile,
        .orientation = cell.orientation + orientation,
    };
}

// create a mesh and draw a grid of hexagonal cells on it
void EditorCursor::build_y_grid() {
    // create the points that make up the top of a hex cell
    Vector<Vector3> shape_points;
    shape_points.append(Vector3(0.0, 0, -1.0));
    shape_points.append(Vector3(-SQRT3_2, 0, -0.5));
    shape_points.append(Vector3(-SQRT3_2, 0, 0.5));
    shape_points.append(Vector3(0.0, 0, 1.0));
    shape_points.append(Vector3(SQRT3_2, 0, 0.5));
    shape_points.append(Vector3(SQRT3_2, 0, -0.5));
    shape_points.append(Vector3(0.0, 0, -1.0));

    // pick a point on the middle of an edge to find the closest edge of the
    // cells
    float max = HexMapCellId(GRID_RADIUS, -(int)GRID_RADIUS / 2, 0)
                        .unit_center()
                        .length_squared();
    PackedVector3Array grid_points;
    PackedColorArray grid_colors;
    for (const auto cell : HexMapCellId::Origin.get_neighbors(
                 GRID_RADIUS, HexMap::Planes::QRS)) {
        Vector3 center = cell.unit_center();
        float transparency =
                Math::pow(MAX(0, (max - center.length_squared()) / max), 2);

        for (int j = 1; j < shape_points.size(); j++) {
            grid_points.append(center + shape_points[j - 1]);
            grid_points.append(center + shape_points[j]);
            grid_colors.push_back(Color(1, 1, 1, transparency));
            grid_colors.push_back(Color(1, 1, 1, transparency));
        }
    }

    Array d;
    d.resize(RS::ARRAY_MAX);
    d[RS::ARRAY_VERTEX] = grid_points;
    d[RS::ARRAY_COLOR] = grid_colors;

    RenderingServer *rs = RenderingServer::get_singleton();
    rs->mesh_add_surface_from_arrays(
            grid_mesh, RenderingServer::PRIMITIVE_LINES, d);
    rs->mesh_surface_set_material(grid_mesh, 0, grid_mat->get_rid());
}

void EditorCursor::build_r_grid() {
    // create the points that traverse from the center of one edge to the
    // opposite edge
    Array shape_points;
    shape_points.append(Vector3(-SQRT3_2, 1.0, 0));
    shape_points.append(Vector3(-SQRT3_2, 0, 0));
    shape_points.append(Vector3(SQRT3_2, 0, 0));
    shape_points.append(Vector3(SQRT3_2, 1.0, 0));
    shape_points.append(Vector3(-SQRT3_2, 1.0, 0));

    float max = HexMapCellId(0, 0, GRID_RADIUS).unit_center().length_squared();
    PackedVector3Array grid_points;
    PackedColorArray grid_colors;
    for (const auto cell : HexMapCellId::Origin.get_neighbors(
                 GRID_RADIUS, HexMap::Planes::YQS)) {
        Vector3 center = cell.unit_center();
        float transparency =
                Math::pow(MAX(0, (max - center.length_squared()) / max), 2);

        for (int j = 1; j < shape_points.size(); j++) {
            grid_points.append(center + shape_points[j - 1]);
            grid_points.append(center + shape_points[j]);
            grid_colors.push_back(Color(1, 1, 1, transparency));
            grid_colors.push_back(Color(1, 1, 1, transparency));
        }
    }

    Array d;
    d.resize(RS::ARRAY_MAX);
    d[RS::ARRAY_VERTEX] = grid_points;
    d[RS::ARRAY_COLOR] = grid_colors;

    RenderingServer *rs = RenderingServer::get_singleton();
    rs->mesh_add_surface_from_arrays(
            grid_mesh, RenderingServer::PRIMITIVE_LINES, d);
    rs->mesh_surface_set_material(grid_mesh, 0, grid_mat->get_rid());
}

void EditorCursor::transform_meshes() {
    RenderingServer *rs = RS::get_singleton();
    Ref<MeshLibrary> mesh_library = hex_map->get_mesh_library();
    Transform3D global_transform = hex_map->get_global_transform();

    Vector3 origin = hex_map->cell_id_to_local(pointer_cell);

    for (CursorCell &cell : tiles) {
        Transform3D transform;
        transform.origin = origin;
        transform.basis = (Basis)this->orientation;
        transform.translate_local(hex_map->cell_id_to_local(cell.cell_id));

        // apply the tile rotation
        transform.basis = (Basis)cell.orientation * transform.basis;

        cell.cell_id_live = hex_map->local_to_cell_id(transform.origin);

        assert(cell.cell_id.y < 0 || origin.y < 0 || transform.origin.y >= 0);

        transform *= mesh_library->get_item_mesh_transform(cell.tile);

        rs->instance_set_transform(
                cell.mesh_instance, global_transform * transform);
    }

    // update the grid transform also
    grid_mesh_transform.set_origin(hex_map->cell_id_to_local(pointer_cell));
    RenderingServer::get_singleton()->instance_set_transform(
            grid_mesh_instance,
            hex_map->get_global_transform() * grid_mesh_transform);
}

bool EditorCursor::update(const Camera3D *camera,
        const Point2 &pointer,
        Vector3 *point) {
    ERR_FAIL_COND_V_MSG(
            camera == nullptr, false, "null camera in EditorCursor.update()");
    Transform3D local_transform =
            hex_map->get_global_transform().affine_inverse();
    Vector3 origin = camera->project_ray_origin(pointer);
    Vector3 normal = camera->project_ray_normal(pointer);
    origin = local_transform.xform(origin);
    normal = local_transform.basis.xform(normal).normalized();

    Vector3 pos;
    if (!edit_plane.intersects_ray(origin, normal, &pos)) {
        return false;
    }
    last_update_origin = origin;
    last_update_normal = normal;

    if (point != nullptr) {
        *point = pos;
    }

    HexMapCellId cell = hex_map->local_to_cell_id(pos);
    if (cell == pointer_cell) {
        return true;
    }
    pointer_cell = cell;
    pointer_pos = pos;

    transform_meshes();

    return true;
}

void EditorCursor::update(bool force) {
    Vector3 pos;
    if (!edit_plane.intersects_ray(
                last_update_origin, last_update_normal, &pos)) {
        return;
    }

    HexMapCellId cell = hex_map->local_to_cell_id(pos);
    if (!force && cell == pointer_cell) {
        return;
    }
    pointer_cell = cell;
    pointer_pos = pos;

    transform_meshes();
}

void EditorCursor::hide() {
    RenderingServer *rs = RS::get_singleton();
    for (CursorCell &cell : tiles) {
        rs->instance_set_visible(cell.mesh_instance, false);
    }
}
void EditorCursor::show() {
    RenderingServer *rs = RS::get_singleton();
    for (CursorCell &cell : tiles) {
        rs->instance_set_visible(cell.mesh_instance, true);
    }
}

void EditorCursor::cell_size_changed() {
    set_axis(edit_axis);
    transform_meshes();
}

void EditorCursor::set_axis(EditorControl::EditAxis axis) {
    // some axis values may have rotated the grid mesh transform.  We want to
    // reset that rotation while preserving the origin.
    grid_mesh_transform = Transform3D();

    // clear the grid mesh
    RenderingServer *rs = RenderingServer::get_singleton();
    rs->mesh_clear(grid_mesh);

    switch (axis) {
        case EditorControl::AXIS_Y:
            build_y_grid();
            edit_plane.normal = Vector3(0, 1, 0);
            break;
        case EditorControl::AXIS_Q:
            build_r_grid();
            grid_mesh_transform.rotate(Vector3(0, 1, 0), -Math_PI / 3.0);
            edit_plane.normal = Vector3(SQRT3_2, 0, -0.5).normalized();
            break;
        case EditorControl::AXIS_R:
            build_r_grid();
            edit_plane.normal = Vector3(0, 0, 1);
            break;
        case EditorControl::AXIS_S:
            build_r_grid();
            grid_mesh_transform.rotate(Vector3(0, 1, 0), Math_PI / 3.0);
            edit_plane.normal = Vector3(SQRT3_2, 0, 0.5).normalized();
            break;
    }

    grid_mesh_transform.scale(hex_map->get_cell_scale());
    RenderingServer::get_singleton()->instance_set_transform(
            grid_mesh_instance, grid_mesh_transform);

    edit_axis = axis;
    update();
}

void EditorCursor::set_depth(int depth) {
    Vector3 cell_scale = hex_map->get_cell_scale();
    real_t cell_depth;

    switch (edit_axis) {
        case EditorControl::AXIS_Y:
            // the y plane is at the bottom of the cell, but to avoid floating
            // point errors during raycast, we pull it slightly higher into the
            // cell.
            edit_plane.d = depth * cell_scale.y + (cell_scale.y * 0.1);
            break;
        case EditorControl::AXIS_Q:
        case EditorControl::AXIS_R:
        case EditorControl::AXIS_S:
            // Q/R/S plane goes through the center of the cell, so no floating
            // point concerns here.
            edit_plane.d = depth * cell_scale.x * 1.5;
            break;
    }
    update();
}

void EditorCursor::set_orientation(TileOrientation orientation) {
    this->orientation = orientation;
    transform_meshes();
}

EditorCursor::EditorCursor(HexMap *map) {
    RenderingServer *rs = RenderingServer::get_singleton();

    grid_mat.instantiate();
    grid_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
    grid_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
    grid_mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
    grid_mat->set_flag(
            StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    grid_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
    grid_mat->set_albedo(Color(0.8, 0.5, 0.1));

    grid_mesh = rs->mesh_create();

    RID scenario = EditorInterface::get_singleton()
                           ->get_editor_viewport_3d()
                           ->get_tree()
                           ->get_root()
                           ->get_world_3d()
                           ->get_scenario();
    grid_mesh_instance = rs->instance_create2(grid_mesh, scenario);
    // 24 = Node3DEditorViewport::MISC_TOOL_LAYER
    rs->instance_set_layer_mask(grid_mesh_instance, 1 << 24);

    hex_map = map;

    set_axis(edit_axis);
    set_depth(0);
}

EditorCursor::~EditorCursor() {
    free_tile_meshes();

    // free the grid meshes
    RenderingServer *rs = RenderingServer::get_singleton();
    rs->free_rid(grid_mesh);
    rs->free_rid(grid_mesh_instance);
}

#endif
