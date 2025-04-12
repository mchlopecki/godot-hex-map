#pragma once

#include <cassert>
#include <climits>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include "core/tile_orientation.h"
#include "godot_cpp/classes/mesh_library.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "int_node/int_node.h"
#include "tiled_node/tiled_node.h"

using namespace godot;

class HexMapAutoTiledNodeEditorPlugin;

class HexMapAutoTiledNode : public Node3D {
    using HexMapAutoTiled = HexMapAutoTiledNode;
    GDCLASS(HexMapAutoTiled, Node3D);

    friend HexMapAutoTiledNodeEditorPlugin;

    /// rule id used to denote that the rule does not have an id
    static const uint16_t RuleIdNotSet = USHRT_MAX;

public:
    class HexMapTileRule;

    /// class defining a single rule that matches some pattern of tile types
    /// in the IntNode, and if it matches, draws a specific tile & orientation.
    class Rule {
        friend HexMapAutoTiledNode;

        /// number of cells contained in the rule pattern
        static const unsigned PatternCells = 19;

        // clang-format off
        /// Used to rotate the cell pattern based on TileOrientation, each
        /// array is a specific tile orientation, and the values within the
        /// array are the pattern index for each cell we're matching against.
        /// So effectively pattern[PatternIndex[rotation][i]] == cells[i] for
        /// any upright rotation value.
        static_assert(HexMapTileOrientation::Upright0 == 0);
        static_assert(HexMapTileOrientation::Upright60 == 1);
        static_assert(HexMapTileOrientation::Upright120 == 2);
        static_assert(HexMapTileOrientation::Upright180 == 3);
        static_assert(HexMapTileOrientation::Upright240 == 4);
        static_assert(HexMapTileOrientation::Upright300 == 5);
        static constexpr uint8_t PatternIndex[6][PatternCells] = {
            // Upright0
            { 0,
              1, 2, 3, 4, 5, 6,
              7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 },
            // Upright60
            { 0,
              2, 3, 4, 5, 6, 1,
              9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 7, 8 },
            // Upright120
            { 0,
              3, 4, 5, 6, 1, 2,
              11, 12, 13, 14, 15, 16, 17, 18, 7, 8, 9, 10 },
            // Upright180
            { 0,
              4, 5, 6, 1, 2, 3,
              13, 14, 15, 16, 17, 18, 7, 8, 9, 10, 11, 12 },
            // Upright240
            { 0,
              5, 6, 1, 2, 3, 4,
              15, 16, 17, 18, 7, 8, 9, 10, 11, 12, 13, 14 },
            // Upright300
            { 0,
              6, 1, 2, 3, 4, 5,
              17, 18, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 },
        };
        static constexpr HexMapCellId CellOffsets[PatternCells] = {
            HexMapCellId(),

            // radius = 1
            HexMapCellId(-1,  1, 0),
            HexMapCellId( 0,  1, 0),
            HexMapCellId( 1,  0, 0),
            HexMapCellId( 1, -1, 0),
            HexMapCellId( 0, -1, 0),
            HexMapCellId(-1,  0, 0),

            // radius = 2
            HexMapCellId(-2,  2, 0),
            HexMapCellId(-1,  2, 0),
            HexMapCellId( 0,  2, 0),
            HexMapCellId( 1,  1, 0),
            HexMapCellId( 2,  0, 0),
            HexMapCellId( 2, -1, 0),
            HexMapCellId( 2, -2, 0),
            HexMapCellId( 1, -2, 0),
            HexMapCellId( 0, -2, 0),
            HexMapCellId(-1, -1, 0),
            HexMapCellId(-2,  0, 0),
            HexMapCellId(-2,  1, 0),
        };
        // clang-format on

    public:
        inline operator Ref<HexMapTileRule>() const;

        void clear_cell(HexMapCellId offset);
        void set_cell_type(HexMapCellId offset, unsigned type);

        /// check if the rule matches the provided cell data with the given
        /// rotation
        /// @param cell_data
        /// @param orientation
        /// @return -1 if matches, >= 0 for the index of mismatch
        inline int match(int32_t cell_type[PatternCells],
                HexMapTileOrientation orientation) {
            int o = static_cast<int>(orientation);
            assert(o >= 0 && o < 6 && "orientation must be in 0..5 inclusive");
            auto *pattern_index = PatternIndex[static_cast<int>(orientation)];

            for (int i = 0; i < PatternCells; i++) {
                const auto &cell = pattern[pattern_index[i]];
                if (!cell.set) {
                    continue;
                }
                if (cell_type[i] != pattern[pattern_index[i]].type) {
                    return i;
                }
            }

            return -1;
        }

    private:
        /// internal rule id, used for ordering
        uint16_t id = RuleIdNotSet;

        struct {
            // 00 - disabled
            // 01 - no type -- specific type == -1
            // 10 - specific type
            // 11 - not specific type
            // enabled/disabled
            // no type
            // specific type
            // not specific type
            int16_t type;
            /// set to 1 when this cell has been set
            unsigned set : 1;
            /// used to invert the state of this cell
            /// nt = 1, set = 1, tile = 5, means not tile 5
            /// nt = 1, set = 0 means tile not set
            /// nt = 0, set = 1, tile = N, means tile N
            unsigned nt : 1;
        } pattern[PatternCells];
        int16_t tile = 0;
    };

    /// GDScript wrapper for Rule class
    class HexMapTileRule : public RefCounted {
        GDCLASS(HexMapTileRule, RefCounted);
        friend HexMapAutoTiledNode;

    public:
        // property getter/setters
        int get_tile() const;
        void set_tile(int value);
        unsigned get_id() const;
        void set_id(unsigned value);

        void clear_cell(const Ref<hex_bind::HexMapCellId> &);
        void set_cell_type(const Ref<hex_bind::HexMapCellId> &, unsigned type);

        HexMapTileRule() {};
        HexMapTileRule(const Rule &rule) : inner(rule) {};

        /// actual Rule value
        Rule inner;

    protected:
        static void _bind_methods();
    };

    HexMapAutoTiledNode();
    ~HexMapAutoTiledNode();

    // HexMapNode::CellInfo get_cell(const HexMapCellId &) const;
    // Array get_cell_ids_v() const;
    // void set_cell_visibility(const HexMapCellId &cell_id, bool visibility);

    // property setters & getters
    void set_mesh_library(const Ref<MeshLibrary> &);
    Ref<MeshLibrary> get_mesh_library() const;
    Dictionary get_rules() const;
    Array get_rules_order() const;

    /// add a new rule; id of new rule will be returned
    unsigned add_rule(const Rule &);
    unsigned add_rule(const Ref<HexMapTileRule> &);
    /// update an existing rule
    void update_rule(const Rule &);
    void update_rule(const Ref<HexMapTileRule> &);
    /// delete a rule by id
    void delete_rule(uint16_t);

    // signal callbacks
    void cell_scale_changed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

    // save/load support
    // void _get_property_list(List<PropertyInfo> *p_list) const;
    // bool _get(const StringName &p_name, Variant &r_ret) const;
    // bool _set(const StringName &p_name, const Variant &p_value);

private:
    void apply_rules();

    Ref<MeshLibrary> mesh_library;
    HexMapTiledNode *tiled_node;
    HexMapIntNode *int_node;

    /// all defined rules
    HashMap<uint16_t, Rule> rules;

    /// order in which the rules should be applied
    Vector<int> rules_order;
};

inline HexMapAutoTiledNode::Rule::operator Ref<
        HexMapAutoTiledNode::HexMapTileRule>() const {
    return Ref<HexMapAutoTiledNode::HexMapTileRule>(
            memnew(HexMapAutoTiledNode::HexMapTileRule(*this)));
}
