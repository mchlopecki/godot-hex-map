extends GutTest

var from_values_params = [
	[0, 0, 0],
	[100, 200, 300],
	[100, 200, -300],
	[100, -200, 300],
	[100, -200, -300],
	[-100, 200, 300],
	[-100, 200, -300],
	[-100, -200, 300],
	[-100, -200, -300],
]

func test_from_coordinates(params=use_parameters(from_values_params)):
	var b := HexMapCellId.from_coordinates(params[0], params[1], params[2]);
	assert_eq(b.get_q(), params[0])
	assert_eq(b.get_r(), params[1])
	assert_eq(b.get_y(), params[2])

# helper used for all following tests
func cell_id(values: Array) -> HexMapCellId:
	return HexMapCellId.from_coordinates(values[0], values[1], values[2])

var equals_params = [
	[[0, 0, 0], [0, 0, 0], true],
	[[0, 0, 0], [0, 0, 1], false],
	[[-1, -1, -1], [-1, -1, -1], true],
]

func test_equals(params=use_parameters(equals_params)):
	var a := cell_id(params[0])
	var b := cell_id(params[1])
	assert_eq(a.equals(b), params[2])

func test_as_int(params=use_parameters(from_values_params)):
	var a := cell_id(params)
	var b := cell_id(params)
	assert_eq(a.as_int(), b.as_int())
	var c := cell_id([1,2,3])
	assert_ne(a.as_int(), c.as_int())

func test_from_int(params=use_parameters(from_values_params)):
	var cell := cell_id(params)
	var value := cell.as_int()
	var from_int := HexMapCellId.from_int(value)
	assert_eq(from_int.get_q(), cell.get_q());
	assert_eq(from_int.get_r(), cell.get_r());
	assert_eq(from_int.get_y(), cell.get_y());

var get_neighbors_params =  from_values_params + [
	# [0, 0, 32767]
]

func test_get_neighbors(params=use_parameters(get_neighbors_params)):
	var cell := cell_id(params)

	var neighbors = []
	for neighbor in cell.get_neighbors():
		neighbors.append(neighbor.as_vector())
		assert_lt(neighbors.size(), 20)

	assert_does_not_have(neighbors, cell.as_vector())
	assert_eq(neighbors.size(), 8)
	assert_has(neighbors, cell.east().as_vector())
	assert_has(neighbors, cell.northeast().as_vector())
	assert_has(neighbors, cell.northwest().as_vector())
	assert_has(neighbors, cell.west().as_vector())
	assert_has(neighbors, cell.southwest().as_vector())
	assert_has(neighbors, cell.southeast().as_vector())
	assert_has(neighbors, cell.up().as_vector())
	assert_has(neighbors, cell.down().as_vector())


