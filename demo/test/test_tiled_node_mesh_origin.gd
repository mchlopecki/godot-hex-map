extends HexMapTest

var cell_origin_params = ParameterFactory.named_parameters(
    ["radius", "height", "cell"], [
        [1, 1, HexMapCellId.at(0, 0, 0)],
        [1, 10, HexMapCellId.at(0, 0, 0)],
        [1, 10, HexMapCellId.at(1, 1, 1)],
        [12, 3, HexMapCellId.at(1, 1, -10)],
    ]
)
func test_get_cell_origin(params=use_parameters(cell_origin_params)):
    var node := HexMapTiled.new()
    node.cell_height = params.height
    node.cell_radius = params.radius

    var center := node.get_cell_center(params.cell)

    # this is an awful workaround because I keep getting GDScrip errors about
    # Value of type "HexMapTiled.MeshOrigin" cannot be assigned to a variable
    # of type "MeshOrigin"
    node.mesh_origin = int(HexMapTiled.MESH_ORIGIN_TOP)
    assert_eq(node.get_cell_origin(params.cell),
        center + Vector3(0, params.height * 0.5, 0))

    node.mesh_origin = int(HexMapTiled.MESH_ORIGIN_BOTTOM)
    assert_eq(node.get_cell_origin(params.cell),
        center + Vector3(0, params.height * -0.5, 0))
    node.mesh_origin = int(HexMapTiled.MESH_ORIGIN_CENTER)
    assert_eq(node.get_cell_origin(params.cell), center)
