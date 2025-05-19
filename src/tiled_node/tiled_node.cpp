#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_library.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_server3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/physics_material.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/pair.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "core/cell_id.h"
#include "core/hex_map_node.h"
#include "core/iter_cube.h"
#include "core/math.h"
#include "core/tile_orientation.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/variant/string.hpp"
#include "octant.h"
#include "profiling.h"
#include "tiled_node.h"

bool HexMapTiledNode::_set(const StringName &p_name, const Variant &p_value) {
    String name = p_name;

    if (name == "data") {
        Dictionary d = p_value;

        if (d.has("cells")) {
            const PackedByteArray cells = d["cells"];
            ERR_FAIL_COND_V(cells.size() % 10 != 0, false);

            size_t offset = 0;
            while (offset < cells.size()) {
                CellId::Key key;
                key.q = cells.decode_s16(offset);
                offset += 2;
                key.r = cells.decode_s16(offset);
                offset += 2;
                key.y = cells.decode_s16(offset);
                offset += 2;

                Cell cell;
                cell.cell = cells.decode_u32(offset);
                offset += 4;

                cell_map[key] = cell;
            }
        }

        recreate_octant_data();
    } else if (name == "baked_meshes") {
        clear_baked_meshes();

        Array meshes = p_value;
        for (int i = 0; i < meshes.size(); i += 2) {
            OctantKey key(meshes[i]);
            ERR_CONTINUE_MSG(!octants.has(key),
                    "octant not found for baked mesh index");
            octants.get(key)->set_baked_mesh(meshes[i + 1]);
            baked_mesh_octants.push_back(key);
        }

        update_octant_meshes();

    } else if (name == "mesh_origin") {
        set_mesh_origin(MeshOrigin(static_cast<int>(p_value)));
    } else {
        return false;
    }

    return true;
}

bool HexMapTiledNode::_get(const StringName &p_name, Variant &r_ret) const {
    String name = p_name;

    if (name == "data") {
        Dictionary d;

        PackedByteArray cells;
        cells.resize(cell_map.size() * 10);
        size_t offset = 0;
        for (const KeyValue<CellKey, Cell> &E : cell_map) {
            cells.encode_s16(offset, E.key.q);
            offset += 2;
            cells.encode_s16(offset, E.key.r);
            offset += 2;
            cells.encode_s16(offset, E.key.y);
            offset += 2;
            cells.encode_u32(offset, E.value.cell);
            offset += 4;
        }

        d["cells"] = cells;

        r_ret = d;
    } else if (name == "baked_meshes") {
        Array ret;
        ret.resize(baked_mesh_octants.size() * 2);
        for (int i = 0; i < baked_mesh_octants.size(); i++) {
            OctantKey key = baked_mesh_octants[i];
            ERR_CONTINUE_MSG(!octants.has(key),
                    "octant not found for baked mesh index");
            ret[2 * i] = key.key;
            ret[2 * i + 1] = octants[key]->get_baked_mesh();
        }
        r_ret = ret;
    } else if (name == "mesh_origin") {
        r_ret = static_cast<int>(mesh_origin);
    } else {
        return false;
    }

    return true;
}

void HexMapTiledNode::_get_property_list(List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::INT,
            "mesh_origin",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
    p_list->push_back(PropertyInfo(Variant::DICTIONARY,
            "data",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
    p_list->push_back(PropertyInfo(Variant::ARRAY,
            "baked_meshes",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_STORAGE));
}

void HexMapTiledNode::set_collision_debug(bool value) {
    if (collision_debug == value) {
        return;
    }
    collision_debug = value;

    for (const auto &it : octants) {
        it.value->exit_world();
        it.value->enter_world();
    }
}

bool HexMapTiledNode::get_collision_debug() const { return collision_debug; }

void HexMapTiledNode::set_collision_layer(uint32_t p_layer) {
    collision_layer = p_layer;
    for (const auto &it : octants) {
        it.value->update_collision_properties();
    }
}

uint32_t HexMapTiledNode::get_collision_layer() const {
    return collision_layer;
}

void HexMapTiledNode::set_collision_mask(uint32_t p_mask) {
    collision_mask = p_mask;
    for (const auto &it : octants) {
        it.value->update_collision_properties();
    }
}

uint32_t HexMapTiledNode::get_collision_mask() const { return collision_mask; }

void HexMapTiledNode::set_collision_layer_value(int p_layer_number,
        bool p_value) {
    ERR_FAIL_COND_MSG(p_layer_number < 1,
            "Collision layer number must be between 1 and 32 inclusive.");
    ERR_FAIL_COND_MSG(p_layer_number > 32,
            "Collision layer number must be between 1 and 32 inclusive.");
    uint32_t collision_layer_new = get_collision_layer();
    if (p_value) {
        collision_layer_new |= 1 << (p_layer_number - 1);
    } else {
        collision_layer_new &= ~(1 << (p_layer_number - 1));
    }
    set_collision_layer(collision_layer_new);
}

bool HexMapTiledNode::get_collision_layer_value(int p_layer_number) const {
    ERR_FAIL_COND_V_MSG(p_layer_number < 1,
            false,
            "Collision layer number must be between 1 and 32 inclusive.");
    ERR_FAIL_COND_V_MSG(p_layer_number > 32,
            false,
            "Collision layer number must be between 1 and 32 inclusive.");
    return get_collision_layer() & (1 << (p_layer_number - 1));
}

void HexMapTiledNode::set_collision_mask_value(int p_layer_number,
        bool p_value) {
    ERR_FAIL_COND_MSG(p_layer_number < 1,
            "Collision layer number must be between 1 and 32 inclusive.");
    ERR_FAIL_COND_MSG(p_layer_number > 32,
            "Collision layer number must be between 1 and 32 inclusive.");
    uint32_t mask = get_collision_mask();
    if (p_value) {
        mask |= 1 << (p_layer_number - 1);
    } else {
        mask &= ~(1 << (p_layer_number - 1));
    }
    set_collision_mask(mask);
}

void HexMapTiledNode::set_collision_priority(real_t p_priority) {
    collision_priority = p_priority;
    for (const auto &it : octants) {
        it.value->update_collision_properties();
    }
}

real_t HexMapTiledNode::get_collision_priority() const {
    return collision_priority;
}

// PhysicsMaterial.computed_friction() & computed_bounce() not exposed in
// godot-cpp
#define computed_friction(mat)                                                \
    ((mat)->is_rough() ? -mat->get_friction() : mat->get_friction())
#define computed_bounce(mat)                                                  \
    ((mat)->is_absorbent() ? -mat->get_bounce() : mat->get_bounce())
void HexMapTiledNode::set_physics_material(Ref<PhysicsMaterial> p_material) {
    physics_material = p_material;
    physics_body_friction = 1.0;
    physics_body_bounce = 0.0;

    if (physics_material.is_valid()) {
        physics_body_friction = computed_friction(physics_material);
        physics_body_bounce = computed_bounce(physics_material);
    }

    for (const auto &it : octants) {
        it.value->update_physics_params();
    }
}

Ref<PhysicsMaterial> HexMapTiledNode::get_physics_material() const {
    return physics_material;
}

bool HexMapTiledNode::get_collision_mask_value(int p_layer_number) const {
    ERR_FAIL_COND_V_MSG(p_layer_number < 1,
            false,
            "Collision layer number must be between 1 and 32 inclusive.");
    ERR_FAIL_COND_V_MSG(p_layer_number > 32,
            false,
            "Collision layer number must be between 1 and 32 inclusive.");
    return get_collision_mask() & (1 << (p_layer_number - 1));
}

void HexMapTiledNode::set_mesh_library(
        const Ref<MeshLibrary> &p_mesh_library) {
    if (!mesh_library.is_null()) {
        mesh_library->disconnect("changed",
                callable_mp(this, &HexMapTiledNode::on_mesh_library_changed));
    }
    mesh_library = p_mesh_library;
    if (!mesh_library.is_null()) {
        mesh_library->connect("changed",
                callable_mp(this, &HexMapTiledNode::on_mesh_library_changed));
    }
    on_mesh_library_changed();
}

Ref<MeshLibrary> HexMapTiledNode::get_mesh_library() const {
    return mesh_library;
}

bool HexMapTiledNode::on_mesh_library_changed() {
    emit_signal("mesh_library_changed");
    for (const auto &iter : octants) {
        iter.value->set_dirty();
    }
    update_dirty_octants();
    return true;
}

void HexMapTiledNode::set_mesh_origin(HexMapTiledNode::MeshOrigin value) {
    if (mesh_origin == value) {
        return;
    }

    mesh_origin = value;

    // mark all octants as dirty, and schedule a refresh of them
    for (const auto pair : octants) {
        pair.value->set_dirty();
    }
    update_dirty_octants();
    emit_signal("mesh_origin_changed");
}

HexMapTiledNode::MeshOrigin HexMapTiledNode::get_mesh_origin() const {
    return mesh_origin;
}

Vector3 HexMapTiledNode::get_mesh_origin_vec() const {
    switch (mesh_origin) {
    case MESH_ORIGIN_CENTER:
        return Vector3();
    case MESH_ORIGIN_TOP:
        return Vector3(0, 0.5, 0);
    case MESH_ORIGIN_BOTTOM:
        return Vector3(0, -0.5, 0);
    }
}

bool HexMapTiledNode::on_hex_space_changed() {
    HexMapNode::on_hex_space_changed();
    clear_baked_meshes();
    for (const auto &iter : octants) {
        iter.value->set_dirty();
    }
    update_dirty_octants();
    return true;
}

void HexMapTiledNode::set_octant_size(int p_size) {
    ERR_FAIL_COND(p_size == 0);
    octant_size = p_size;
    recreate_octant_data();
}

int HexMapTiledNode::get_octant_size() const { return octant_size; }

void HexMapTiledNode::set_navigation_bake_only_navmesh_tiles(bool value) {
    navigation_bake_only_navmesh_tiles = value;
}

bool HexMapTiledNode::get_navigation_bake_only_navmesh_tiles() const {
    return navigation_bake_only_navmesh_tiles;
}

void HexMapTiledNode::set_cell(const HexMapCellId &cell_id,
        int value,
        HexMapTileOrientation orientation) {
    auto prof = profiling_begin("set cell item");
    ERR_FAIL_COND_MSG(
            !cell_id.in_bounds(), "cell id is not in bounds: " + cell_id);

    // Convert the cell id into a key.  If the cell is already set to the
    // specified values, just return.
    CellKey cell_key(cell_id);
    Cell *current_cell = cell_map.getptr(cell_key);
    if (current_cell != nullptr && value == current_cell->value &&
            orientation == current_cell->rot) {
        return;
    }

    // if the octant meshes have been baked, we need to clear them now.  Even
    // if we only clear the modified octant, the HexMap will look odd with the
    // lightmap only applied to one section.
    if (!baked_mesh_octants.is_empty()) {
        UtilityFunctions::print(
                "HexMap: map modified; clearing baked lighting meshes");
        clear_baked_meshes();
    }

    // look up the cell octant
    OctantKey octant_key(cell_id, octant_size);
    Octant **octant_ptr = octants.getptr(octant_key);
    Octant *octant = octant_ptr ? *octant_ptr : nullptr;

    if (value >= 0) {
        // set the cell
        Cell cell = {
            .value = static_cast<unsigned int>(value),
            .rot = static_cast<unsigned int>(orientation),
            .visible = true,
        };
        cell_map.insert(cell_key, cell);

        // create a new octant if one doesn't already exist for this cell
        if (octant == nullptr) {
            octant = new Octant(*this);
            octants.insert(octant_key, octant);

            if (is_inside_tree()) {
                octant->enter_world();
            }
        }

        // add a cell to the octant, and schedule an update
        octant->set_cell(cell_key, value, orientation);
        update_dirty_octants();

    } else if (current_cell != nullptr) {
        // clear the cell
        cell_map.erase(cell_key);

        ERR_FAIL_COND_MSG(octant == nullptr, "octant for cell does not exist");
        octant->clear_cell(cell_key);
        update_dirty_octants();
    }

    return;
}

Array HexMapTiledNode::get_cell_vecs() const {
    Array out;
    for (const auto &iter : cell_map) {
        out.push_back((Vector3i)iter.key);
    }
    return out;
}

Array HexMapTiledNode::find_cell_vecs_by_value(int value) const {
    Array out;
    for (const auto &iter : cell_map) {
        if (iter.value.value == value) {
            out.push_back(static_cast<Vector3i>(iter.key));
        }
    }
    return out;
}

HexMapNode::CellInfo HexMapTiledNode::get_cell(
        const HexMapCellId &cell_id) const {
    const Cell *current_cell = cell_map.getptr(cell_id);
    if (current_cell == nullptr) {
        return CellInfo{ .value = CELL_VALUE_NONE };
    }
    return CellInfo{ .value = static_cast<int>(current_cell->value),
        .orientation = HexMapTileOrientation(current_cell->rot) };
}

void HexMapTiledNode::set_cell_visibility(const HexMapCellId &cell_id,
        bool visibility) {
    ERR_FAIL_COND_MSG(
            !cell_id.in_bounds(), "cell id is not in bounds:" + cell_id);

    // Convert the cell id into a key.  If the cell is already set to the
    // specified values, just return.
    CellKey cell_key(cell_id);
    Cell *cell = cell_map.getptr(cell_key);
    if (cell == nullptr) {
        return;
    }
    cell->visible = visibility;

    OctantKey octant_key(cell_id, octant_size);
    Octant **octant = octants.getptr(octant_key);
    ERR_FAIL_COND_MSG(
            octant == nullptr, "no octant found for valid cell: " + cell_id);
    (**octant).set_cell_visibility(cell_id, visibility);
    update_dirty_octants();

    // Although we aren't truely modifying the map here, covering the edge case
    // of hide/unhide cells and restoring the baked mesh feels like a lot of
    // work.  For now we're just gonna clear the baked meshes when they start
    // hiding cells.
    if (!baked_mesh_octants.is_empty()) {
        UtilityFunctions::print(
                "HexMap: map modified; clearing baked lighting meshes");
        clear_baked_meshes();
    }

    return;
}

void HexMapTiledNode::_notification(int p_what) {
    static Transform3D last_transform;

    switch (p_what) {
    case NOTIFICATION_ENTER_WORLD:
        UtilityFunctions::print(
                "tiled node entering world with ", octants.size(), " octants");
        for (auto &pair : octants) {
            pair.value->enter_world();
        }
        last_transform = get_global_transform();
        break;

    case NOTIFICATION_ENTER_TREE:
        UtilityFunctions::print(
                "tiled node enter tree with ", octants.size(), " octants");
        _update_visibility();
        break;

    case NOTIFICATION_TRANSFORM_CHANGED: {
        Transform3D transform = get_global_transform();
        if (transform != last_transform) {
            space.set_transform(transform);
            update_octant_meshes();
            last_transform = transform;
        }
        break;
    }
    case NOTIFICATION_EXIT_WORLD:
        for (auto &pair : octants) {
            pair.value->exit_world();
        }
        break;

    case NOTIFICATION_VISIBILITY_CHANGED:
        _update_visibility();
        break;
    }
}

void HexMapTiledNode::_update_visibility() {
    if (!is_inside_tree()) {
        return;
    }

    for (auto &pair : octants) {
        pair.value->update_visibility();
    }
}

void HexMapTiledNode::update_dirty_octants_callback() {
    ERR_FAIL_COND_MSG(!awaiting_update,
            "update_dirty_octants_callback() called unexpectedly");
    Vector<OctantKey> empty_octants;
    for (const auto pair : octants) {
        Octant *octant = pair.value;
        if (!octant->is_dirty()) {
            continue;
        }

        octant->apply_changes();
        if (octant->is_empty()) {
            empty_octants.push_back(pair.key);
        } else {
            // update visibility for dirty, non-empty octants
            octant->update_visibility();
        }
    }

    for (const auto key : empty_octants) {
        delete octants[key];
        octants.erase(key);
    }

    awaiting_update = false;
}

void HexMapTiledNode::update_dirty_octants() {
    if (awaiting_update) {
        return;
    }
    awaiting_update = true;
    callable_mp(this, &HexMapTiledNode::update_dirty_octants_callback)
            .call_deferred();
}

void HexMapTiledNode::update_octant_meshes() {
    for (auto &it : octants) {
        it.value->apply_changes();
    }
}

void HexMapTiledNode::recreate_octant_data() {
    HashMap<CellKey, Cell> cell_copy = cell_map;
    clear_internal();
    for (const KeyValue<CellKey, Cell> &E : cell_copy) {
        set_cell(CellId(E.key), E.value.value, E.value.rot);
    }
}

void HexMapTiledNode::clear_internal() {
    for (auto &octant_pair : octants) {
        Octant *octant = octant_pair.value;
        if (is_inside_tree()) {
            octant->exit_world();
        }
        delete octant;
    }
    octants.clear();
    cell_map.clear();
}

void HexMapTiledNode::clear() {
    clear_internal();
    clear_baked_meshes();
}

void HexMapTiledNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_mesh_library", "mesh_library"),
            &HexMapTiledNode::set_mesh_library);
    ClassDB::bind_method(
            D_METHOD("get_mesh_library"), &HexMapTiledNode::get_mesh_library);

    ClassDB::bind_method(D_METHOD("set_mesh_origin", "value"),
            &HexMapTiledNode::set_mesh_origin);
    ClassDB::bind_method(
            D_METHOD("get_mesh_origin"), &HexMapTiledNode::get_mesh_origin);

    ClassDB::bind_method(D_METHOD("set_collision_debug", "debug"),
            &HexMapTiledNode::set_collision_debug);
    ClassDB::bind_method(D_METHOD("get_collision_debug"),
            &HexMapTiledNode::get_collision_debug);

    ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"),
            &HexMapTiledNode::set_collision_layer);
    ClassDB::bind_method(D_METHOD("get_collision_layer"),
            &HexMapTiledNode::get_collision_layer);

    ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"),
            &HexMapTiledNode::set_collision_mask);
    ClassDB::bind_method(D_METHOD("get_collision_mask"),
            &HexMapTiledNode::get_collision_mask);

    ClassDB::bind_method(
            D_METHOD("set_collision_mask_value", "layer_number", "value"),
            &HexMapTiledNode::set_collision_mask_value);
    ClassDB::bind_method(D_METHOD("get_collision_mask_value", "layer_number"),
            &HexMapTiledNode::get_collision_mask_value);

    ClassDB::bind_method(
            D_METHOD("set_collision_layer_value", "layer_number", "value"),
            &HexMapTiledNode::set_collision_layer_value);
    ClassDB::bind_method(D_METHOD("get_collision_layer_value", "layer_number"),
            &HexMapTiledNode::get_collision_layer_value);

    ClassDB::bind_method(D_METHOD("set_collision_priority", "priority"),
            &HexMapTiledNode::set_collision_priority);
    ClassDB::bind_method(D_METHOD("get_collision_priority"),
            &HexMapTiledNode::get_collision_priority);

    ClassDB::bind_method(D_METHOD("set_physics_material", "material"),
            &HexMapTiledNode::set_physics_material);
    ClassDB::bind_method(D_METHOD("get_physics_material"),
            &HexMapTiledNode::get_physics_material);

    ClassDB::bind_method(
            D_METHOD("set_navigation_bake_only_navmesh_tiles", "enable"),
            &HexMapTiledNode::set_navigation_bake_only_navmesh_tiles);
    ClassDB::bind_method(D_METHOD("get_navigation_bake_only_navmesh_tiles"),
            &HexMapTiledNode::get_navigation_bake_only_navmesh_tiles);

    ClassDB::bind_method(D_METHOD("set_octant_size", "size"),
            &HexMapTiledNode::set_octant_size);
    ClassDB::bind_method(
            D_METHOD("get_octant_size"), &HexMapTiledNode::get_octant_size);

    ClassDB::bind_method(D_METHOD("clear"), &HexMapTiledNode::clear);

    ClassDB::bind_method(
            D_METHOD("get_bake_meshes"), &HexMapTiledNode::get_bake_meshes);
    ClassDB::bind_method(D_METHOD("get_bake_mesh_instance", "idx"),
            &HexMapTiledNode::get_bake_mesh_instance);
    ClassDB::bind_method(D_METHOD("clear_baked_meshes"),
            &HexMapTiledNode::clear_baked_meshes);

    ClassDB::bind_method(D_METHOD("make_baked_meshes",
                                 "gen_lightmap_uv",
                                 "lightmap_uv_texel_size"),
            &HexMapTiledNode::make_baked_meshes,
            DEFVAL(false),
            DEFVAL(0.1));

    ClassDB::bind_method(D_METHOD("get_cell_origin", "cell_id"),
            &HexMapTiledNode::get_cell_origin);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
                         "mesh_library",
                         PROPERTY_HINT_RESOURCE_TYPE,
                         "MeshLibrary"),
            "set_mesh_library",
            "get_mesh_library");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
                         "physics_material",
                         PROPERTY_HINT_RESOURCE_TYPE,
                         "PhysicsMaterial"),
            "set_physics_material",
            "get_physics_material");

    ADD_GROUP("Cell", "cell_");
    ADD_PROPERTY(PropertyInfo(Variant::INT,
                         "cell_octant_size",
                         PROPERTY_HINT_RANGE,
                         "1,1024,1"),
            "set_octant_size",
            "get_octant_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT,
                         "mesh_origin",
                         PROPERTY_HINT_ENUM,
                         "Center,Top,Bottom"),
            "set_mesh_origin",
            "get_mesh_origin");

    ADD_GROUP("Collision", "collision_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL,
                         "collision_debug",
                         PROPERTY_HINT_LAYERS_3D_PHYSICS),
            "set_collision_debug",
            "get_collision_debug");
    ADD_PROPERTY(PropertyInfo(Variant::INT,
                         "collision_layer",
                         PROPERTY_HINT_LAYERS_3D_PHYSICS),
            "set_collision_layer",
            "get_collision_layer");
    ADD_PROPERTY(PropertyInfo(Variant::INT,
                         "collision_mask",
                         PROPERTY_HINT_LAYERS_3D_PHYSICS),
            "set_collision_mask",
            "get_collision_mask");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_priority"),
            "set_collision_priority",
            "get_collision_priority");
    ADD_GROUP("Navigation", "navigation_");

    ADD_PROPERTY(
            PropertyInfo(Variant::BOOL, "navigation_bake_only_navmesh_tiles"),
            "set_navigation_bake_only_navmesh_tiles",
            "get_navigation_bake_only_navmesh_tiles");

    BIND_CONSTANT(CELL_VALUE_NONE);

    ADD_SIGNAL(MethodInfo(
            "cell_changed", PropertyInfo(Variant::VECTOR3I, "cell")));
    ADD_SIGNAL(MethodInfo("mesh_library_changed"));

    ADD_SIGNAL(MethodInfo("mesh_origin_changed"));

    BIND_ENUM_CONSTANT(MESH_ORIGIN_CENTER);
    BIND_ENUM_CONSTANT(MESH_ORIGIN_BOTTOM);
    BIND_ENUM_CONSTANT(MESH_ORIGIN_TOP);
}

void HexMapTiledNode::clear_baked_meshes() {
    for (auto &it : octants) {
        it.value->clear_baked_mesh();
    }
    baked_mesh_octants.clear();
}

void HexMapTiledNode::make_baked_meshes(bool p_gen_lightmap_uv,
        float p_lightmap_uv_texel_size) {}

Array HexMapTiledNode::get_bake_meshes() {
    baked_mesh_octants.clear();
    Array arr;
    for (auto &it : octants) {
        baked_mesh_octants.push_back(it.key);
        arr.push_back(it.value->get_baked_mesh());
        arr.push_back(Transform3D());
    }
    return arr;
}

RID HexMapTiledNode::get_bake_mesh_instance(int p_idx) {
    ERR_FAIL_INDEX_V(p_idx, baked_mesh_octants.size(), RID());
    OctantKey key = baked_mesh_octants[p_idx];
    ERR_FAIL_COND_V_MSG(
            !octants.has(key), RID(), "octant not found for baked mesh index");
    return octants.get(key)->get_baked_mesh_instance();
}

bool HexMapTiledNode::generate_navigation_source_geometry(Ref<NavigationMesh>,
        Ref<NavigationMeshSourceGeometryData3D> source_geometry_data,
        Node *) const {
    // calculate the mesh origin offset for each cell; this allows us to put
    // the origin at the bottom or top of the cell, instead of the center.
    Vector3 mesh_origin_offset =
            get_mesh_origin_vec() * space.get_cell_scale();

    for (const auto &it : cell_map) {
        const Cell &cell = it.value;

        if (navigation_bake_only_navmesh_tiles) {
            // If there's a tile in the cell above this one, do not include
            // this tile, otherwise when a navigable mesh has a
            // non-navigable on top, the nav mesh incorrectly cuts through
            // that upper tile.
            if (it.key.y < SHRT_MAX && cell_map.has(it.key.get_cell_above())) {
                continue;
            }

            // if the cell doesn't have a navmesh, skip it
            if (!mesh_library->get_item_navigation_mesh(cell.value)
                            .is_valid()) {
                continue;
            }
        }

        Ref<Mesh> mesh = mesh_library->get_item_mesh(cell.value);
        if (!mesh.is_valid()) {
            continue;
        }

        Transform3D cell_transform(cell.get_basis(),
                space.get_cell_center_global(it.key) + mesh_origin_offset);
        source_geometry_data->add_mesh(mesh,
                cell_transform *
                        mesh_library->get_item_mesh_transform(cell.value));
    }

    // Unused return value.  To turn this function into a Callable, the
    // return type needs to be able to be converted into a Variant.  `void`
    // is not a supported option.
    return true;
}

Vector3 HexMapTiledNode::get_cell_origin(
        Ref<hex_bind::HexMapCellId> ref) const {
    Vector3 out = space.get_cell_center(ref->inner);
    out += get_mesh_origin_vec() * space.get_cell_scale();
    return out;
}

HexMapTiledNode::HexMapTiledNode() {
    set_notify_transform(true);

    // copied from SceneTree::get_debug_collision_material()
    collision_debug_mat.instantiate();
    collision_debug_mat->set_shading_mode(
            StandardMaterial3D::SHADING_MODE_UNSHADED);
    collision_debug_mat->set_transparency(
            StandardMaterial3D::TRANSPARENCY_ALPHA);
    collision_debug_mat->set_flag(
            StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
    collision_debug_mat->set_flag(
            StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    collision_debug_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
    collision_debug_mat->set_albedo(
            ProjectSettings::get_singleton()->get_setting_with_override(
                    "debug/shapes/collision/shape_color"));

    // set up the source geometry parser for navmesh generation
    NavigationServer3D *ns = NavigationServer3D::get_singleton();
    navigation_source_geometry_parser = ns->source_geometry_parser_create();
    ns->source_geometry_parser_set_callback(navigation_source_geometry_parser,
            callable_mp(this,
                    &HexMapTiledNode::generate_navigation_source_geometry));
}

HexMapTiledNode::~HexMapTiledNode() {
    clear();
    NavigationServer3D::get_singleton()->free_rid(
            navigation_source_geometry_parser);
}
