extends HexMapTest

func parse_cell_map(buf: String) -> Array:
    # split the buffer into lines, stripping common indent, comments, and blank
    # lines
    var lines:= []
    for line in buf.dedent().split("\n"):
        if line.begins_with("#") or line.is_empty():
            continue
        lines.append(line)

    var pos := HexMapCellId.new()
    var row_start := HexMapCellId.new()
    var origin : HexMapCellId = null
    var out := []

    for line in lines:
        if line[0] == "v":
            if not out.is_empty():
                pos = row_start.southwest()
                row_start = pos
            continue
        if line[0] == "^":
            if not out.is_empty():
                pos = row_start.southeast()
                row_start = pos
            continue


        for text in line.strip_edges().split("|", false):
            text = text.strip_edges()
            if text.begins_with("_O"):
                origin = pos
                text = text.substr(2,-1).strip_edges()

            out.push_back({"cell": pos, "text": text.strip_edges()})
            pos = pos.east()

    if origin:
        for entry in out:
            entry["cell"] = entry["cell"].subtract(origin)

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
    ["empty tile (radius = 2) with rotation","
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
])

func test_auto_tiled_rules(params=use_parameters(rules_params)):
    # build the int_node
    print("int node:\n", params.int_node.dedent());
    var int_node := HexMapInt.new()
    for cell in parse_cell_map(params.int_node):
        if !cell["text"].is_empty():
            int_node.set_cell(cell["cell"], int(cell["text"]))

    # create the auto tiled node, add it as a child of the int node, then add
    # required rules
    var auto_node := HexMapAutoTiled.new()
    for input in params.rules:
        var rule := HexMapTileRule.new()
        rule.tile = input["tile"]
        for cell in parse_cell_map(input["cells"]):
            if cell["text"] == "!":
                # XXX impl
                rule.set_cell_empty(cell["cell"])
            elif cell["text"] != "":
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
            var tile = int(cell["text"])
            assert_node_cell_value_eq(auto_node, cell_id, tile)

    int_node.remove_child(auto_node)
    auto_node.free()
    int_node.free()

    pass


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
            elif cell["text"] != "":
                rule.set_cell_type(cell["cell"], int(cell["text"]))
        print(rule)
        auto_node.add_rule(rule)

    var iteration_begin_usec = Time.get_ticks_usec()
    # make the auto_node a child of int node
    int_node.add_child(auto_node)
    var duration = Time.get_ticks_usec() - iteration_begin_usec

    pass_test(str("rules took ", duration, "us to complete"))
    int_node.free()


