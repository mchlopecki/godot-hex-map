#pragma once

#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/color.hpp>

#include "core/cell_id.h"
#include "core/hex_map_node.h"

using namespace godot;

class HexMapIntNodeEditorPlugin;
class HexMapAutoTiledNode;

class HexMapIntNode : public HexMapNode {
    using HexMapInt = HexMapIntNode;
    GDCLASS(HexMapInt, HexMapNode);

    friend HexMapIntNodeEditorPlugin;
    friend HexMapAutoTiledNode;

public:
    struct CellType {
        String name;
        Color color;
    };
    using TypeMap = HashMap<unsigned, CellType>;

    /// Argument to set_cell_type to have the cell type id assigned
    /// automatically
    static const int TypeIdNext = -1;

    /// insert or update a cell type
    unsigned set_cell_type(unsigned id, const String, const Color);

    /// remove a tile type
    void remove_cell_type(unsigned id);

    /// get a const reference to the types set for this hexmap node
    inline const TypeMap &get_cell_types() const { return cell_types; };

    /// return an Array of Dictionary with value, name, color keys
    Array _get_cell_types() const;

    /// Look up a specific cell type
    Variant get_cell_type(unsigned id) const;

    /// all of the required override functions
    void set_cell(const HexMapCellId &,
            int tile,
            HexMapTileOrientation orientation = 0) override;
    CellInfo get_cell(const HexMapCellId &) const override;
    Array get_cell_vecs() const override;
    Array find_cell_vecs_by_value(int value) const override;
    void set_cell_visibility(const HexMapCellId &cell_id,
            bool visibility) override {};

protected:
    // void _notification(int p_what);
    static void _bind_methods();
    void _get_property_list(List<PropertyInfo> *p_list) const;
    bool _get(const StringName &p_name, Variant &r_ret) const;
    bool _set(const StringName &p_name, const Variant &p_value);

private:
    unsigned type_id_max;
    TypeMap cell_types;
    HashMap<HexMapCellId::Key, uint16_t> cell_map;
};
