extends GutTest
class_name HexMapTest

func CellId(q,r,y):
	return HexMapCellId.from_coordinates(q,r,y)

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
