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

func test_from_values(params=use_parameters(from_values_params)):
	var b := HexMapCellId.from_values(params[0], params[1], params[2]);
	assert_eq(b.get_q(), params[0])
	assert_eq(b.get_r(), params[1])
	assert_eq(b.get_y(), params[2])

# helper used for all following tests
func cell_id(values: Array) -> HexMapCellId:
	return HexMapCellId.from_values(values[0], values[1], values[2])

var equals_params = [
	[[0, 0, 0], [0, 0, 0], true],
	[[0, 0, 0], [0, 0, 1], false],
	[[-1, -1, -1], [-1, -1, -1], true],
]

func test_equals(params=use_parameters(equals_params)):
	var a := cell_id(params[0])
	var b := cell_id(params[1])
	assert_eq(a.equals(b), params[2])


func test_hash(params=use_parameters(from_values_params)):
	var a := cell_id(params)
	var b := cell_id(params)
	assert_eq(a.hash(), b.hash())
	var c := cell_id([1,2,3])
	assert_ne(a.hash(), c.hash())

func test_from_hash(params=use_parameters(from_values_params)):
	var cell := cell_id(params)
	var h := cell.hash()
	var from_hash := HexMapCellId.from_hash(h)
	assert_eq(from_hash.get_q(), cell.get_q());
	assert_eq(from_hash.get_r(), cell.get_r());
	assert_eq(from_hash.get_y(), cell.get_y());

var get_neighbors_params =  from_values_params + [
	# [0, 0, 32767]
]

func test_get_neighbors(params=use_parameters(get_neighbors_params)):
	var cell := cell_id(params)

	var neighbors = []
	for neighbor in cell.get_neighbors():
		print("neighbor ", neighbor)
		neighbors.append(neighbor.hash())
		assert_lt(neighbors.size(), 20)

	assert_does_not_have(neighbors, cell.hash())
	assert_eq(neighbors.size(), 8)
	assert_has(neighbors, cell.east().hash())
	assert_has(neighbors, cell.northeast().hash())
	assert_has(neighbors, cell.northwest().hash())
	assert_has(neighbors, cell.west().hash())
	assert_has(neighbors, cell.southwest().hash())
	assert_has(neighbors, cell.southeast().hash())
	assert_has(neighbors, cell.up().hash())
	assert_has(neighbors, cell.down().hash())


