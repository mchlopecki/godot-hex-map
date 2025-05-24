extends GutTest
class_name HexMapTest

func CellId(q,r,y):
	return HexMapCellId.at(q,r,y)

func cell_center(q,r,y):
	return CellId(q,r,y).unit_center();

func assert_cells_eq(found, expect) -> void:
	var _found = []
	for cell in found:
		_found.push_back(cell.as_int())
	_found.sort()

	var _expect = []
	for cell in expect:
		_expect.push_back(cell.as_int())
	_expect.sort()

	if _found == _expect:
		pass_test("expected cells to match")
		return

	var missing = []
	var extra = []

	for id in _expect:
		if !_found.has(id):
			missing.push_back(HexMapCellId.from_int(id))
	for id in _found:
		if !_expect.has(id):
			extra.push_back(HexMapCellId.from_int(id))

	fail_test(str("cells do not match; missing: ", missing, ", extra: ", extra))

func assert_cell_eq(found, expect) -> void:
	if found.as_int() == expect.as_int():
		pass_test(str("expected cell to equal ", expect))
		return

	fail_test(str("cells are not equal; expected: ", expect, ", got: ", found))

func assert_node_cell_value_eq(node, cell_id: HexMapCellId, type: int) -> void:
	var found = node.get_cell(cell_id)
	if found["value"] == type:
		pass_test(str("expected cell ", cell_id, " type to equal ", type))
	else:
		fail_test(str("cell ", cell_id, " type incorrect; expected ", type,
			", found: ", found["value"]))

func assert_node_cell_eq(node, cell_id: HexMapCellId, type: int, orientation = 0) -> void:
	var found = node.get_cell(cell_id)
	var error
	if found.value != type:
		error = str("cell ", cell_id, " type incorrect; expected ", type,
			", found: ", found["value"])
	elif orientation && found.orientation != orientation:
		error = str("cell ", cell_id, " orientation incorrect; expected ",
			orientation, ", found: ", found.orientation)
	else:
		pass_test(str("expected cell ", cell_id, " type to equal ", [type, orientation]))
		return

	error += "\nnode.cells = [\n"
	var cells = node.get_cells(node.get_cell_vecs())
	var count : int = cells.size()/node.CELL_ARRAY_WIDTH
	for i in range(count):
		var base = i * node.CELL_ARRAY_WIDTH
		error += "    { %s, %d, %d },\n" % [
			HexMapCellId.from_vec(cells[base + node.CELL_ARRAY_INDEX_VEC]),
			cells[base + node.CELL_ARRAY_INDEX_VALUE],
			cells[base + node.CELL_ARRAY_INDEX_ORIENTATION]]

	fail_test(error)
