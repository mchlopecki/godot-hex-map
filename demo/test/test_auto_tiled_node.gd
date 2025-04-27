extends HexMapTest

func parse_cell_map_layer(lines: Array, y: int) -> Array:
    # split the buffer into lines, stripping common indent, comments, and blank
    # lines
    var pos := HexMapCellId.at(0, 0, y)
    var row_start := HexMapCellId.at(0, 0, y)
    var origin : HexMapCellId = null
    var out := []

    while not lines.is_empty():
        if lines[0][0] == "y":
            break
        var line = lines.pop_front()

        if line[0] == "v":
            if pos.q != 0 || pos.r != 0:
                pos = row_start.southwest()
                row_start = pos
            continue
        if line[0] == "^":
            if pos.q != 0 || pos.r != 0:
                pos = row_start.southeast()
                row_start = pos
            continue

        for text in line.strip_edges().split("|", false):
            text = text.strip_edges()
            if text.begins_with("_O"):
                origin = HexMapCellId.at(pos.q, pos.r, 0)
                text = text.substr(2,-1).strip_edges()

            out.push_back({"cell": pos, "text": text.strip_edges()})
            pos = pos.east()

    if origin:
        for entry in out:
            entry["cell"] = entry["cell"].subtract(origin)

    return out

func parse_cell_map(buf: String) -> Array:
    # split the buffer into lines, stripping common indent, comments, and blank
    # lines
    var lines:= []
    for line in buf.dedent().split("\n"):
        if line.begins_with("#") or line.is_empty():
            continue
        lines.append(line)

    var out := []
    var y := 0

    while not lines.is_empty():
        # if the next line is y=, parse it and pop the line
        if lines[0][0] == "y":
            var line = lines.pop_front()
            y = int(line.get_slice("=", 1))

        # parse the remaining lines as a layer
        out.append_array(parse_cell_map_layer(lines, y))

    return out


var cell_parser_params = ParameterFactory.named_parameters(
    ["desc", "map", "results"], [
    [
        "single cell is at origin, text contains spaces",
        "
         v---------^--------v
         | text with spaces |
         ^---------v--------^",
        [
            { "cell": HexMapCellId.new(), "text": "text with spaces" },
        ],
    ], [
        "two cells, side by side, no origin specified",
        "
v---^---v---^---v
| left  | right |
^---v---^---v---^
        ", [
            { "cell": HexMapCellId.new(), "text": "left" },
            { "cell": HexMapCellId.new().east(), "text": "right" },
        ],
    ], [
        "multiple cells, with origin specified, comments, and indented",
        "
    # ignore this comment
    ^---v--^---v--^---v
        | nw   | ne   |
    # another comment for some reason
    v---^--v---^--v---^--v
    | w    | _O o | e    |
    ^---v--^---v--^---v--^
        | sw   | se   |
    v---^--v---^--v---^
    # closing comments
        ", [
            { "cell": HexMapCellId.new().northwest(), "text": "nw" },
            { "cell": HexMapCellId.new().northeast(), "text": "ne" },
            { "cell": HexMapCellId.new().west(), "text": "w" },
            { "cell": HexMapCellId.new(), "text": "o" },
            { "cell": HexMapCellId.new().east(), "text": "e" },
            { "cell": HexMapCellId.new().southwest(), "text": "sw" },
            { "cell": HexMapCellId.new().southeast(), "text": "se" },
        ],
    ], [
        "multiple cells, with origin specified, comments, and indented",
        "
            ^---v--^---v--^---v
                | !    | !    |
            v---^--v---^--v---^--v
            | !    | _O 5 | !    |
            ^---v--^---v--^---v--^
        ", [
            { "cell": HexMapCellId.new().northwest(), "text": "!" },
            { "cell": HexMapCellId.new().northeast(), "text": "!" },
            { "cell": HexMapCellId.new().west(), "text": "!" },
            { "cell": HexMapCellId.new(), "text": "5" },
            { "cell": HexMapCellId.new().east(), "text": "!" },
        ],
    ], [
        "complex example with empty cells",
        "
            v---^--v---^--v---^--v---^--v
            |      | 9    | !    | 9    |
            ^---v--^---v--^---v--^---v--^---v
                | !    | !    | !    | !    |
            v---^--v---^--v---^--v---^--v---^--v
            | 9    | !    | _O ! | !    | 9    |
            ^---v--^---v--^---v--^---v--^---v--^
                | !    | !    | !    | !    |
            v---^--v---^--v---^--v---^--v---^
            |      | 9    | !    | 9    |
            ^---v--^---v--^---v--^---v--^
        ", [
            { "cell": HexMapCellId.at(-1, -2), "text": "" },
            { "cell": HexMapCellId.at( 0, -2), "text": "9" },
            { "cell": HexMapCellId.at( 1, -2), "text": "!" },
            { "cell": HexMapCellId.at( 2, -2), "text": "9" },

            { "cell": HexMapCellId.at(-1, -1), "text": "!" },
            { "cell": HexMapCellId.at( 0, -1), "text": "!" },
            { "cell": HexMapCellId.at( 1, -1), "text": "!" },
            { "cell": HexMapCellId.at( 2, -1), "text": "!" },

            { "cell": HexMapCellId.at(-2,  0), "text": "9" },
            { "cell": HexMapCellId.at(-1,  0), "text": "!" },
            { "cell": HexMapCellId.at( 0,  0), "text": "!" },
            { "cell": HexMapCellId.at( 1,  0), "text": "!" },
            { "cell": HexMapCellId.at( 2,  0), "text": "9" },

            { "cell": HexMapCellId.at(-2,  1), "text": "!" },
            { "cell": HexMapCellId.at(-1,  1), "text": "!" },
            { "cell": HexMapCellId.at( 0,  1), "text": "!" },
            { "cell": HexMapCellId.at( 1,  1), "text": "!" },

            { "cell": HexMapCellId.at(-3,  2), "text": "" },
            { "cell": HexMapCellId.at(-2,  2), "text": "9" },
            { "cell": HexMapCellId.at(-1,  2), "text": "!" },
            { "cell": HexMapCellId.at( 0,  2), "text": "9" },
        ],
    ], [
        "multiple layers",
        "
            y = 0
            v----^---v
            | origin |
            ^----v---^
            y = 1
            v--^--v
            | up  |
            ^--v--^
            y = -1
            v---^--v
            | down |
            ^---v--^

        ", [
            { "cell": HexMapCellId.at(0, 0, 0), "text": "origin" },
            { "cell": HexMapCellId.at(0, 0, 1), "text": "up" },
            { "cell": HexMapCellId.at(0, 0, -1), "text": "down" },
        ],
    ], [
        "multiple offset layers",
        "
            y = 0
            v------^------v
            | zero_origin |
            ^------v------^------v
                   | zero_se     |
            v------^------v------^
            y = 1
            v-------^-------v-------^-------v
            | _O one_origin | one_east      |
            ^-------v-------^-------v-------^
            y = 2
            v-------^-------v-------^-------v
            | two_west      | _O two_origin |
            ^-------v-------^-------v-------^
            y = 3
            v--------^--------v
            | three_nw        |
            ^--------v--------^--------v
                     | _O three_origin |
            v--------^--------v--------^

        ", [
            { "cell": HexMapCellId.at(0, 0, 0), "text": "zero_origin" },
            { "cell": HexMapCellId.at(0, 1, 0), "text": "zero_se" },
            { "cell": HexMapCellId.at(0, 0, 1), "text": "one_origin" },
            { "cell": HexMapCellId.at(1, 0, 1), "text": "one_east" },
            { "cell": HexMapCellId.at(-1, 0, 2), "text": "two_west" },
            { "cell": HexMapCellId.at(0, 0, 2), "text": "two_origin" },
            { "cell": HexMapCellId.at(0, -1, 3), "text": "three_nw" },
            { "cell": HexMapCellId.at(0, 0, 3), "text": "three_origin" },
        ],
    ],

])

func test_cell_parser(params=use_parameters(cell_parser_params)):
    var output = parse_cell_map(params.map)
    for i in range(params.results.size()):
        var found = output[i]
        var expected = params.results[i]
        assert_eq(found["text"], expected["text"])
        assert_cell_eq(found["cell"], expected["cell"])
    pass

var rules_params = ParameterFactory.named_parameters(
    ["desc", "int_node", "rules", "expected"], [
    ["single cell replacement rules", "
            v--^--v--^--v--^--v
            | 1   | 2   | 3   |
            ^--v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
               v--^--v
               | 1   |
               ^--v--^
            "},
            {
            "tile": 0,
            "cells": "
               v--^--v
               | 2   |
               ^--v--^
            "},
            # this rule should never match as its exactly the same as the
            # previous rule
            {
            "tile": 222,
            "cells": "
               v--^--v
               | 2   |
               ^--v--^
            "},
        ], "
          v--^--v--^--v--^--v
          | 5   | 0   | -1  |
          ^--v--^--v--^--v--^
        "
    ],
    ["not type rule", "
            v--^--v--^--v--^--v
            | 1   | 2   | 3   |
            ^--v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
               v--^--v
               | !1  |
               ^--v--^
            "},
        ], "
          v--^--v--^--v--^--v
          | -1  | 5   | 5   |
          ^--v--^--v--^--v--^
        "
    ],
    ["not empty rule", "
            v--^--v--^--v--^--v
            | 1   |     | 3   |
            ^--v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
               v--^--v
               |  +  |
               ^--v--^
            "},
        ], "
          v--^--v--^--v--^--v
          |  5  | -1  | 5   |
          ^--v--^--v--^--v--^
        "
    ],
    ["neighbor-based pattern","
            ^--v--^--v--^--v
               |     |     |
            v--^--v--^--v--^--v
            | 1   | 0   |     |
            ^--v--^--v--^--v--^
               |     |     |
            v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                v---^--v---^--v
                | 1    | _O 0 |
                ^---v--^---v--^
            "},
        ], "
            ^--v--^--v--^--v
               | !   | !   |
            v--^--v--^--v--^--v
            | !   | 5   | !   |
            ^--v--^--v--^--v--^
        "
    ],
    ["neighbor-based pattern, rotate 60","
            ^--v--^--v--^--v
               |     |     |
            v--^--v--^--v--^--v
            |     | 0   |     |
            ^--v--^--v--^--v--^
               | 1   |     |
            v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                v---^--v---^--v
                | 1    | _O 0 |
                ^---v--^---v--^
            "},
            # Add a rule after that matches the actual orientation to ensure
            # that rotation is matched before moving to the next rule
            {
            "tile": 999,
            "cells": "
               ^--v--^--v--^
                  | _O 0   |
               v--^--v--^--v
               | 1   |
               ^--v--^
            "},
        ], "
            ^--v--^--v--^--v
               | !   | !   |
            v--^--v--^--v--^--v
            | !   | 5   | !   |
            ^--v--^--v--^--v--^
        "
    ],
    ["neighbor-based pattern, rotate 120","
            ^--v--^--v--^--v
               |     |     |
            v--^--v--^--v--^--v
            |     | 0   |     |
            ^--v--^--v--^--v--^
               |     |  1  |
            v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                v---^--v---^--v
                | 1    | _O 0 |
                ^---v--^---v--^
            "},
        ], "
            ^--v--^--v--^--v
               | !   | !   |
            v--^--v--^--v--^--v
            | !   | 5   | !   |
            ^--v--^--v--^--v--^
        "
    ],
    ["neighbor-based pattern, rotate 180","
            ^--v--^--v--^--v
               |     |     |
            v--^--v--^--v--^--v
            |     | 0   |  1  |
            ^--v--^--v--^--v--^
               |     |     |
            v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                v---^--v---^--v
                | 1    | _O 0 |
                ^---v--^---v--^
            "},
        ], "
            ^--v--^--v--^--v
               | !   | !   |
            v--^--v--^--v--^--v
            | !   | 5   | !   |
            ^--v--^--v--^--v--^
        "
    ],
    ["neighbor-based pattern, rotate 240","
            ^--v--^--v--^--v
               |     |  1  |
            v--^--v--^--v--^--v
            |     | 0   |     |
            ^--v--^--v--^--v--^
               |     |     |
            v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                v---^--v---^--v
                | 1    | _O 0 |
                ^---v--^---v--^
            "},
        ], "
            ^--v--^--v--^--v
               | !   | !   |
            v--^--v--^--v--^--v
            | !   | 5   | !   |
            ^--v--^--v--^--v--^
        "
    ],
    ["neighbor-based pattern, rotate 300","
            ^--v--^--v--^--v
               |  1  |     |
            v--^--v--^--v--^--v
            |     | 0   |     |
            ^--v--^--v--^--v--^
               |     |     |
            v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                v---^--v---^--v
                | 1    | _O 0 |
                ^---v--^---v--^
            "},
        ], "
            ^--v--^--v--^--v
               | !   | !   |
            v--^--v--^--v--^--v
            | !   | 5   | !   |
            ^--v--^--v--^--v--^
        "
    ],
    ["set tile where there is no value in HexMapInt","
            ^--v--^--v--^--v
               | 1   | 1   |
            v--^--v--^--v--^--v
            |     | _O  |     |
            ^--v--^--v--^--v--^
        ", [{
            "tile": 5,
            "cells": "
                ^---v--^---v--^---v
                    | 1    | 1    |
                v---^--v---^--v---^--v
                | !    | _O ! | !    |
                ^---v--^---v--^---v--^
            "},
        ], "
            ^---v--^---v--^---v
                | !    | !    |
            v---^--v---^--v---^--v
            | !    | _O 5 | !    |
            ^---v--^---v--^---v--^
        "
    ],
    ["empty tile with rotation","
            ^---v--^---v--^---v
                |      |      |
            v---^--v---^--v---^--v
            |      | _O 1 |      |
            ^---v--^---v--^---v--^
                |      |      |
            v---^--v---^--v---^
        ", [{
            "tile": 9,
            "cells": "
                v---^--v---^--v
                | _O ! | 1    |
                ^---v--^---v--^
            "},
        ], "
            ^---v--^---v--^---v
                |  9   |  9   |
            v---^--v---^--v---^--v
            |   9  | _O ! |  9   |
            ^---v--^---v--^---v--^
                |   9  |   9  |
            v---^--v---^--v---^
        "
    ],
    ["empty tile (radius = 2) with all orientations" ,"
            v---^--v
            | _O 1 |
            ^---v--^
        ", [{
            "tile": 9,
            "cells": "
                v---^--v---^--v---^--v
                | _O ! |      | 1    |
                ^---v--^---v--^---v--^
            "},
        ], "
            v---^--v---^--v---^--v---^--v
            | !    | 9    | !    | 9    |
            ^---v--^---v--^---v--^---v--^---v
                | !    | !    | !    | !    |
            v---^--v---^--v---^--v---^--v---^--v
            | 9    | !    | _O ! | !    | 9    |
            ^---v--^---v--^---v--^---v--^---v--^
                | !    | !    | !    | !    |
            v---^--v---^--v---^--v---^--v---^
            |      | 9    | !    | 9    |
            ^---v--^---v--^---v--^---v--^
        "
    ],
    ["empty tile (radius = 2) with one orientation","
        ^---v--^---v--^---v
            | 2    | 1    |
        v---^--v---^--v---^--v
        | 2    | _O 2 | 1    |
        ^---v--^---v--^---v--^
            | 2    | 2    |
        v---^--v---^--v---^
        ", [{
            "tile": 9,
            "cells": "

            ^---v--^---v--^---v
                | 1    | 1    |
            v---^--v---^--v---^--v
            | 2    | _O 2 | 2    |
            ^---v--^---v--^---v--^
                | 2    | 2    |
            v---^--v---^--v---^
            "},
        ], "
            ^-----v-----^-----v-----^-----v
                  | !         | !         |
            v-----^-----v-----^-----v-----^-----v
            | !         | _O 9, -60 | !         |
            ^-----v-----^-----v-----^-----v-----^
                  | !         | !         |
            v-----^-----v-----^-----v-----^
        "
    ],
    ["vertical cells","
        y = 11
        v--^--v
        | 2   |
        ^--v--^

        y = 10
        v--^--v
        | 1   |
        ^--v--^

        y = 9
        v--^--v
        | 0   |
        ^--v--^
        ", [{
            "tile": 9,
            "cells": "
            v--^--v
            | 2   |
            ^--v--^

            y = 0
            v--^--v
            | 1   |
            ^--v--^

            y = -1
            v--^--v
            | 0   |
            ^--v--^
            "},
        ], "
            y = 11
            v--^--v
            | !   |
            ^--v--^

            y = 10
            v--^--v
            | 9   |
            ^--v--^

            y = 9
            v--^--v
            | !   |
            ^--v--^
        "
    ],
    ["vertical pattern with rotation","
        y = 10
        ^---v--^---v--^---v
            | 2    | 1    |
        v---^--v---^--v---^--v
        | 2    | _O 2 | 1    |
        ^---v--^---v--^---v--^
            | 2    | 2    |
        v---^--v---^--v---^
        ", [{
            "tile": 9,
            "cells": "
            y = 0
            v--^--v
            | !   |
            ^--v--^
            y = -1
            ^--v--^--v--^--v
               | 1   | 1   |
            v--^--v--^--v--^--v
            | 2   | _O 2   | 2   |
            ^--v--^--v--^--v--^
               | 2   | 2   |
            v--^--v--^--v--^

            "},
        ], "
            y = 11
            ^-----v-----^-----v-----^-----v
                  | !         | !         |
            v-----^-----v-----^-----v-----^-----v
            | !         | _O 9, -60 | !         |
            ^-----v-----^-----v-----^-----v-----^
                  | !         | !         |
            v-----^-----v-----^-----v-----^
        "
    ],
    # I want to verify that we're correctly expanding our search space to
    # match empty cells.  The rules in this test case all have an empty cell
    # at origin, so we'll have to evaluate the rules in places where we don't
    # have a cell value set in the int node.  This test is to confirm that we
    # properly expand the cell search space based on where the nearest tile
    # can be found for these empty cell rules.
    ["expand search space for empty cells","
        y = 2
        v--^---v
        | _O 5 |
        ^--v---^
        y = 1
        v---^---v---^---v
        | _O 10 | 11    |
        ^---v---^---v---^
        y = 0
        ^--v--^--v--^--v--^--v--^--v--^--v
           | 2   |     | _O  |     | 8   |
        v--^--v--^--v--^--v--^--v--^--v--^--v
        | 1   |     |     |     |     | 9   |
        ^--v--^--v--^--v--^--v--^--v--^--v--^
        ", [
            {
            "tile": 12,
            "cells": "
            ^---v--^---v
                | 2    |
            v---^--v---^--v---^--v
            | 1    |      | _O ! |
            ^---v--^---v--^---v--^
            "},
            {
            "tile": 25,
            "cells": "
            y = 2
            v--^--v
            | 5   |
            ^--v--^
            y = 0
            v--^--v
            |  !  |
            ^--v--^
            "},
            {
            "tile": 1011,
            "cells": "
            y = 1
            v---^---v---^---v
            | 10    | _O 11 |
            ^---v---^---v---^
            y = 0
            v---^--v---^--v
            |      | _O ! |
            ^---v--^---v--^
            "},
        ], "
        y = 0
        ^---v---^---v---^---v---^---v---^---v---^---v
            | !     | !     | _O 25 | 1011  | !     |
        v---^---v---^---v---^---v---^---v---^---v---^---v
        | !     | !     | 12    | !     | !     | !     |
        ^---v---^---v---^---v---^---v---^---v---^---v---^
        "
    ],
])

func test_auto_tiled_rules(params=use_parameters(rules_params)):
    # build the int_node
    print("int node:\n", params.int_node.dedent());
    var int_node := HexMapInt.new()
    for cell in parse_cell_map(params.int_node):
        if !cell["text"].is_empty():
            print("set_cell(", cell, ")")
            int_node.set_cell(cell["cell"], int(cell["text"]))

    # create the auto tiled node, add it as a child of the int node, then add
    # required rules
    var auto_node := HexMapAutoTiled.new()
    # XXX break this out into a func; we use it later for benchmarks
    for input in params.rules:
        var rule := HexMapTileRule.new()
        rule.tile = input["tile"]
        for cell in parse_cell_map(input["cells"]):
            if cell["text"] == "!":
                rule.set_cell_empty(cell["cell"])
            elif cell["text"] == "+":
                rule.set_cell_empty(cell["cell"], true)
            elif cell["text"].begins_with("!"):
                var tile = int(cell["text"].substr(1, -1))
                print("set_cell_type(", cell["cell"], ")")
                rule.set_cell_type(cell["cell"], tile, true)
            elif cell["text"] != "":
                print("set_cell_type(", cell["cell"], ", ", cell["text"], ")")
                rule.set_cell_type(cell["cell"], int(cell["text"]))
        print(rule)
        auto_node.add_rule(rule)
    print("expected:\n", params.expected.dedent());

    # make the auto_node a child of int node
    int_node.add_child(auto_node)

    # verify the rules were applied correctly
    for cell in parse_cell_map(params.expected):
        var cell_id = cell["cell"]

        if cell["text"] == "!":
            assert_node_cell_value_eq(auto_node, cell_id, -1)
        elif cell["text"] != "":
            var words = cell.text.split(", ")
            var tile = int(words[0])
            var orientation = 0
            if words.size() > 1:
                var degree = int(words[1])
                if degree < 0:
                    degree = 360 + degree
                orientation = (int(degree)/60)

            assert_node_cell_eq(auto_node, cell_id, tile, orientation)

    int_node.remove_child(auto_node)
    auto_node.free()
    int_node.free()

# extracted from Rule::CellOffsets, contains every offset present in a Rule
var RuleCellOffsets := [
        HexMapCellId.at( 0,  0,  0),   # 0: origin
        HexMapCellId.at( 0,  0,  1),
        HexMapCellId.at( 0,  0, -1),
        HexMapCellId.at( 0,  0,  2),
        HexMapCellId.at( 0,  0, -2),

        # radius = 1, starting at southwest corner, counter-clockwise
        HexMapCellId.at(-1,  1, 0),    # 5
        HexMapCellId.at( 0,  1, 0),
        HexMapCellId.at( 1,  0, 0),
        HexMapCellId.at( 1, -1, 0),
        HexMapCellId.at( 0, -1, 0),
        HexMapCellId.at(-1,  0, 0),    # 10

        # radius = 1, y = 1
        HexMapCellId.at(-1,  1, 1),    # 11
        HexMapCellId.at( 0,  1, 1),
        HexMapCellId.at( 1,  0, 1),
        HexMapCellId.at( 1, -1, 1),
        HexMapCellId.at( 0, -1, 1),
        HexMapCellId.at(-1,  0, 1),    # 16

        # radius = 1, y = -1
        HexMapCellId.at(-1,  1, -1),   # 17
        HexMapCellId.at( 0,  1, -1),
        HexMapCellId.at( 1,  0, -1),
        HexMapCellId.at( 1, -1, -1),
        HexMapCellId.at( 0, -1, -1),
        HexMapCellId.at(-1,  0, -1),   # 22

        # radius = 2, starting at southwest corner, counter-clockwise
        HexMapCellId.at(-2,  2, 0),    # 23
        HexMapCellId.at(-1,  2, 0),
        HexMapCellId.at( 0,  2, 0),
        HexMapCellId.at( 1,  1, 0),
        HexMapCellId.at( 2,  0, 0),
        HexMapCellId.at( 2, -1, 0),
        HexMapCellId.at( 2, -2, 0),
        HexMapCellId.at( 1, -2, 0),
        HexMapCellId.at( 0, -2, 0),
        HexMapCellId.at(-1, -1, 0),
        HexMapCellId.at(-2,  0, 0),
        HexMapCellId.at(-2,  1, 0),    # 35
    ]

# exhaustive test to verify that every individual rule cell will match when
# rotated
func test_each_rule_cell_matches() -> void:
    # go through all possible rotations
    for orientation in range(6):

        # construct an int node, each coordinate will hold a unique value.
        # This map is rotated by the orientation.
        var int_node := HexMapInt.new()
        var value := 0
        for offset in RuleCellOffsets:
            int_node.set_cell(offset.rotate(orientation), value)
            value += 1

        # test each individual cell in the rule pattern to verify it matches
        value = 0
        for offset in RuleCellOffsets:
            var rule := HexMapTileRule.new()
            rule.tile = 1234
            rule.set_cell_type(offset, value)
            rule.set_cell_type(HexMapCellId.new(), 0)
            # print("orientation ", orientation, ", offset ", offset, ", at ",
            #     offset.rotate(orientation), ", value ", value)
            value += 1

            var auto_node := HexMapAutoTiled.new()
            auto_node.add_rule(rule)

            # add the auto node to the int node, applying all rules
            int_node.add_child(auto_node)

            # verify the cell was properly set
            for cell_id in RuleCellOffsets:
                if cell_id.q == 0 && cell_id.r == 0 && cell_id.y == 0:
                    if offset.q == 0 && offset.r == 0:
                        assert_node_cell_eq(auto_node, cell_id, 1234, 0)
                    else:
                        assert_node_cell_eq(auto_node, cell_id, 1234,
                            orientation)
                else:
                    assert_node_cell_eq(auto_node, cell_id, -1)

            # clean up so GUT doesn't complain of orphans
            int_node.remove_child(auto_node)
            auto_node.free()

var benchmark_rules_params = ParameterFactory.named_parameters(
    ["desc", "map_size", "map_values", "rules"], [
    [
        "1,000,000 map cells, three radius=0 rules that don't match",
         1000000,
        [9],
        [{
            "tile": 5,
            "cells": "
               v--^--v
               | 1   |
               ^--v--^
            "},
            {
            "tile": 0,
            "cells": "
               v--^--v
               | 2   |
               ^--v--^
            "},
            {
            "tile": 222,
            "cells": "
               v--^--v
               | 2   |
               ^--v--^
            "},
        ]
    ], [
        "1,000,000 map cells, three radius=1 rules that don't match",
         1000000,
        [9],
        [{
            "tile": 5,
            "cells": "
               v--^--v--^--v
               | 1   | !   |
               ^--v--^--v--^
            "},
            {
            "tile": 0,
            "cells": "
               v--^--v--^--v
               | 2   | !   |
               ^--v--^--v--^
            "},
            {
            "tile": 222,
            "cells": "
               v--^--v--^--v
               | 3   | !   |
               ^--v--^--v--^
            "},
        ]
    ], [
        "1,000,000 map cells, three radius=2 rules that don't match",
         1000000,
        [9],
        [{
            "tile": 5,
            "cells": "
               v--^--v--^--v--^--v
               | 1   | 2   | 3   |
               ^--v--^--v--^--v--^
            "},
            {
            "tile": 0,
            "cells": "
               v--^--v--^--v--^--v
               | 2   | 3   | 4   |
               ^--v--^--v--^--v--^
            "},
            {
            "tile": 222,
            "cells": "
               v--^--v--^--v--^--v
               | 3   | 4   | 5   |
               ^--v--^--v--^--v--^
            "},
        ]
    ], [
        "1,000,000 map cells, worst case rule r=2, all cells set & match",
         1000000,
        [1],
        [{
            "tile": 1,
            "cells": "
            v---^--v---^--v---^--v---^--v
            |      | 1    | 1    | 1    |
            ^---v--^---v--^---v--^---v--^---v
                | 1    | 1    | 1    | 1    |
            v---^--v---^--v---^--v---^--v---^--v
            | 1    | 1    | _O 1 | 1    | 1    |
            ^---v--^---v--^---v--^---v--^---v--^
                | 1    | 1    | 1    | 1    |
            v---^--v---^--v---^--v---^--v---^
            |      | 1    | 1    | 1    |
            ^---v--^---v--^---v--^---v--^
            "},
        ]
    ], [
        "1,000,000 map cells, worst case border_radius=2 match",
         1000000,
        [1],
        [{
            "tile": 1,
            "cells": "
            # we specifically pick the last cell to be evaluated in match to
            # force worst-case match
            v---^--v---^--v---^--v---^--v
            |      |      |      |      |
            ^---v--^---v--^---v--^---v--^---v
                |      |      |      |      |
            v---^--v---^--v---^--v---^--v---^--v
            |      |      | _O ! |      |      |
            ^---v--^---v--^---v--^---v--^---v--^
                |  1   |      |      |      |
            v---^--v---^--v---^--v---^--v---^
            |      |      |      |      |
            ^---v--^---v--^---v--^---v--^
            "},
        ]
    ],

])

func test_benchmark_auto_tiled_rules(params=use_parameters(benchmark_rules_params)):
    if OS.get_environment("HEX_MAP_RUN_BENCH") != "1":
        pending("benchmarks disabled; set HEX_MAP_RUN_BENCH=1 to enable")
        return

    var int_node = HexMapInt.new()
    var count = 0;
    for cell_id in HexMapCellId.new().get_neighbors(100):
        int_node.set_cell(cell_id, params.map_values[0])
        count += 1
        if count >= params.map_size:
            break

    # create the auto tiled node, add it as a child of the int node, then add
    # required rules
    var auto_node := HexMapAutoTiled.new()
    for input in params.rules:
        var rule := HexMapTileRule.new()
        rule.tile = input["tile"]
        for cell in parse_cell_map(input["cells"]):
            if cell["text"] == "!":
                rule.set_cell_empty(cell["cell"])
            elif cell["text"] == "+":
                rule.set_cell_empty(cell["cell"], true)
            elif cell["text"].begins_with("!"):
                var tile = int(cell["text"].substr(1, -1))
                rule.set_cell_type(cell["cell"], tile, true)
            elif cell["text"] != "":
                rule.set_cell_type(cell["cell"], int(cell["text"]))
        # print(rule)
        auto_node.add_rule(rule)

    var iteration_begin_usec = Time.get_ticks_usec()
    # make the auto_node a child of int node
    int_node.add_child(auto_node)
    var duration = Time.get_ticks_usec() - iteration_begin_usec

    pass_test(str(params.desc, " took ", duration, "us to complete"))
    int_node.free()
