#include "../src/hex_map/hex_map_iter_axial.h"
#include "../src/hex_map/hex_map.h"
#include "doctest.h"

TEST_CASE("HexMapIterAxial::operator++(); prefix increment") {
	HexMapIterAxial iter(HexMapCellId(), 1, HexMap::Planes::All);

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

TEST_CASE("HexMapIterAxial::operator++(); prefix increment @ (10, -20, 30)") {
	HexMapIterAxial iter(HexMapCellId(10, -20, 30), 1, HexMap::Planes::All);

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

TEST_CASE("HexMapIterAxial::operator++(); prefix increment (QRS)") {
	HexMapIterAxial iter(HexMapCellId(), 1, HexMap::Planes::QRS);

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

TEST_CASE("HexMapIterAxial::operator++(); prefix increment (YRS)") {
	HexMapIterAxial iter(HexMapCellId(), 1, HexMap::Planes::YRS);

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

TEST_CASE("HexMapIterAxial::operator++(); prefix increment (YQS)") {
	HexMapIterAxial iter(HexMapCellId(), 1, HexMap::Planes::YQS);

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

TEST_CASE("HexMapIterAxial::operator++(); prefix increment (YQR)") {
	HexMapIterAxial iter(HexMapCellId(), 1, HexMap::Planes::YQR);

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

TEST_CASE("HexMapIterAxial::begin() is within radius") {
	HexMapIterAxial iter(HexMapCellId(), 1);
	CHECK_MESSAGE((*iter.begin()).distance(HexMapCellId()) <= 1,
			"begin cell should be within the radius");
}
