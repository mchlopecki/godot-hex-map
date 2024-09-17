extends GutTest

func test_equals():
	var a := HexMapCellIdRef.new()
	var b := HexMapCellIdRef.new()
	assert_true(a.equals(b))

func test_from_values():
	var b := HexMapCellIdRef.from_values(-1,29,-100)
	assert_eq(b.get_q(), -1)
	assert_eq(b.get_r(), 29)
	assert_eq(b.get_y(), -100)

func test_hash():
	var a := HexMapCellIdRef.from_values(-9,-6,-1)
	var b := HexMapCellIdRef.from_values(-9,-6,-1)
	assert_eq(a.hash(), b.hash())
	b = HexMapCellIdRef.from_values(1,-2,-1)
	assert_ne(a.hash(), b.hash())

func test_from_hash():
	# XXX figure out parameterized tests
	var signs := [
		[1, 1, 1],
		[1, 1, -1],
		[1, -1, 1],
		[1, -1, -1],
		[-1, 1, 1],
		[-1, 1, -1],
		[-1, -1, 1],
		[-1, -1, -1]]
	for adj in signs:
		var cell := HexMapCellIdRef.from_values(12 * adj[0], 41 * adj[1], 1 * adj[2])
		var h := cell.hash()
		var from_hash := HexMapCellIdRef.from_hash(h)
		print("cell ", cell, ", hash ", h, ", from_hash ", from_hash)
		assert_eq(from_hash.get_q(), cell.get_q());
		assert_eq(from_hash.get_r(), cell.get_r());
		assert_eq(from_hash.get_y(), cell.get_y());

func test_get_neighbors():
	# start with origin
	var cell := HexMapCellIdRef.new()

	var neighbors = []
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

