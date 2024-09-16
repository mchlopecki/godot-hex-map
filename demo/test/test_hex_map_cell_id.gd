extends GutTest

func test_equality():
	var a := HexMapCellIdRef.new()
	var b := HexMapCellIdRef.new()
	assert_true(a.equals(b))

func test_hash():
	var a := HexMapCellIdRef.new()
	var b := HexMapCellIdRef.new()
	assert_eq(a.hash(), b.hash())

func test_get_neighbors():
	# start with origin
	var cell := HexMapCellIdRef.new();

	var neighbors = [];
	for neighbor in cell.get_neighbors():
		print(neighbor)
		neighbors.append(neighbor.hash())

	assert_eq(neighbors.size(), 8)
	assert_has(neighbors, cell.east().hash())
	assert_has(neighbors, cell.northeast().hash())
	assert_has(neighbors, cell.northwest().hash())
	assert_has(neighbors, cell.west().hash())
	assert_has(neighbors, cell.southwest().hash())
	assert_has(neighbors, cell.southeast().hash())
	assert_has(neighbors, cell.up().hash())
	assert_has(neighbors, cell.down().hash())

	# try a new location
	cell = HexMapCellIdRef.from_values(100,200,300)
	print(cell)

	neighbors.clear()
	for neighbor in cell.get_neighbors():
		print(neighbor)
		neighbors.append(neighbor.hash())

	assert_eq(neighbors.size(), 8)
	assert_has(neighbors, cell.east().hash())
	assert_has(neighbors, cell.northeast().hash())
	assert_has(neighbors, cell.northwest().hash())
	assert_has(neighbors, cell.west().hash())
	assert_has(neighbors, cell.southwest().hash())
	assert_has(neighbors, cell.southeast().hash())
	assert_has(neighbors, cell.up().hash())
	assert_has(neighbors, cell.down().hash())

