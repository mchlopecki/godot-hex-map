#include "../src/hex_map/hex_map_cell_iterator.h"
#include "../src/hex_map/hex_map.h"
#include "doctest.h"

TEST_CASE("HexMapCellIterator::operator++(); prefix increment") {
	HexMapCellIterator iter(HexMapCellId(), 1, HexMap::Planes::All);

	// confirm first value
	CHECK(*iter == HexMapCellId(0, 0, -1));
	CHECK_MESSAGE(
			*++iter == HexMapCellId(-1, 0, 0), "increment y; reset r & q");
	CHECK(*++iter == HexMapCellId(-1, 1, 0));
	CHECK(*++iter == HexMapCellId(0, -1, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 0));
	CHECK(*++iter == HexMapCellId(0, 1, 0));
	CHECK(*++iter == HexMapCellId(1, -1, 0));
	CHECK(*++iter == HexMapCellId(1, 0, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 1));

	// ensure we can't go past the end.
	CHECK_MESSAGE(*++iter == HexMapCellId(2, 2, 2), "increments to end");
	CHECK_MESSAGE(
			*++iter == HexMapCellId(2, 2, 2), "does not increment past end");
	CHECK_MESSAGE(*iter == *iter.end(), "check the end");
}

TEST_CASE("HexMapCellIterator::operator++(); prefix increment (QRS)") {
	HexMapCellIterator iter(HexMapCellId(), 1, HexMap::Planes::QRS);

	// confirm first value
	CHECK(*iter == HexMapCellId(-1, 0, 0));
	CHECK(*++iter == HexMapCellId(-1, 1, 0));
	CHECK(*++iter == HexMapCellId(0, -1, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 0));
	CHECK(*++iter == HexMapCellId(0, 1, 0));
	CHECK(*++iter == HexMapCellId(1, -1, 0));
	CHECK(*++iter == HexMapCellId(1, 0, 0));

	// ensure we can't go past the end.
	CHECK_MESSAGE(*++iter == *iter.end(), "increments to end");
	CHECK_MESSAGE(*++iter == *iter.end(), "does not increment past end");
}

TEST_CASE("HexMapCellIterator::operator++(); prefix increment (YRS)") {
	HexMapCellIterator iter(HexMapCellId(), 1, HexMap::Planes::YRS);

	// confirm first value
	CHECK(*iter == HexMapCellId(0, 0, -1));
	CHECK(*++iter == HexMapCellId(0, -1, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 0));
	CHECK(*++iter == HexMapCellId(0, 1, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 1));

	// ensure we can't go past the end.
	CHECK_MESSAGE(*++iter == *iter.end(), "increments to end");
	CHECK_MESSAGE(*++iter == *iter.end(), "does not increment past end");
}

TEST_CASE("HexMapCellIterator::operator++(); prefix increment (YQS)") {
	HexMapCellIterator iter(HexMapCellId(), 1, HexMap::Planes::YQS);

	// confirm first value
	CHECK(*iter == HexMapCellId(0, 0, -1));
	CHECK(*++iter == HexMapCellId(-1, 0, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 0));
	CHECK(*++iter == HexMapCellId(1, 0, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 1));

	// ensure we can't go past the end.
	CHECK_MESSAGE(*++iter == *iter.end(), "increments to end");
	CHECK_MESSAGE(*++iter == *iter.end(), "does not increment past end");
}

TEST_CASE("HexMapCellIterator::operator++(); prefix increment (YQR)") {
	HexMapCellIterator iter(HexMapCellId(), 1, HexMap::Planes::YQR);

	// confirm first value
	CHECK(*iter == HexMapCellId(0, 0, -1));
	CHECK(*++iter == HexMapCellId(-1, 1, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 0));
	CHECK(*++iter == HexMapCellId(1, -1, 0));
	CHECK(*++iter == HexMapCellId(0, 0, 1));

	// ensure we can't go past the end.
	CHECK_MESSAGE(*++iter == *iter.end(), "increments to end");
	CHECK_MESSAGE(*++iter == *iter.end(), "does not increment past end");
}

TEST_CASE("HexMapCellIterator::begin() is within radius") {
	HexMapCellIterator iter(HexMapCellId(), 1);
	CHECK_MESSAGE((*iter.begin()).distance(HexMapCellId()) <= 1,
			"begin cell should be within the radius");
}
