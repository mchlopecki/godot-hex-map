#include "../src/hex_map/hex_map_iter_axial.h"
#include "../src/hex_map/hex_map.h"
#include "doctest.h"

enum Dir {
	UP = 1 << 1,
	DOWN = 1 << 2,
	EAST = 1 << 3,
	NE = 1 << 4,
	NW = 1 << 5,
	WEST = 1 << 6,
	SW = 1 << 7,
	SE = 1 << 8,
	ALL = UP | DOWN | EAST | NE | NW | WEST | SW | SE,
};

void check_neighbors(
		HexMapCellId center, HexMap::Planes planes, int directions) {
	CAPTURE(center);
	HexMapIterAxial iter(center, 1, planes);

	if (directions & DOWN) {
		CHECK_MESSAGE(*iter == center + HexMapCellId(0, 0, -1), "down", iter);
		iter++;
	}
	if (directions & WEST) {
		CHECK_MESSAGE(*iter == center + HexMapCellId(-1, 0, 0), "west", iter);
		iter++;
	}
	if (directions & SW) {
		CHECK_MESSAGE(
				*iter == center + HexMapCellId(-1, 1, 0), "southwest", iter);
		iter++;
	}
	if (directions & NW) {
		CHECK_MESSAGE(
				*iter == center + HexMapCellId(0, -1, 0), "northwest", iter);
		iter++;
	}
	if (directions & SE) {
		CHECK_MESSAGE(
				*iter == center + HexMapCellId(0, 1, 0), "southeast", iter);
		iter++;
	}
	if (directions & NE) {
		CHECK_MESSAGE(
				*iter == center + HexMapCellId(1, -1, 0), "northeast", iter);
		iter++;
	}
	if (directions & EAST) {
		CHECK_MESSAGE(*iter == center + HexMapCellId(1, 0, 0), "east", iter);
		iter++;
	}
	if (directions & UP) {
		CHECK_MESSAGE(*iter == center + HexMapCellId(0, 0, 1), "up", iter);
		iter++;
	}
	CHECK_MESSAGE(*++iter == *iter.end(), "should be at the end", iter);
}

TEST_CASE("HexMapIterAxial::operator++()") {
	SUBCASE("center at y-min, won't include down cell") {
		check_neighbors(
				HexMapCellId(0, 0, SHRT_MIN), HexMap::Planes::All, ~DOWN);
	}
	SUBCASE("center at y-max, wont include up cell") {
		check_neighbors(
				HexMapCellId(0, 0, SHRT_MAX), HexMap::Planes::All, ~UP);
	}
	SUBCASE("center at q-min, won't include west/southwest") {
		check_neighbors(HexMapCellId(SHRT_MIN, 0, 0), HexMap::Planes::All,
				~WEST & ~SW);
	}
	SUBCASE("center at q-max, wont include east/northeast") {
		check_neighbors(HexMapCellId(SHRT_MAX, 0, 0), HexMap::Planes::All,
				~EAST & ~NE);
	}
	SUBCASE("center at r-min, won't include northwest/northeast") {
		check_neighbors(
				HexMapCellId(0, SHRT_MIN, 0), HexMap::Planes::All, ~NW & ~NE);
	}
	SUBCASE("center at r-max, wont include southwest/southeast") {
		check_neighbors(
				HexMapCellId(0, SHRT_MAX, 0), HexMap::Planes::All, ~SW & ~SE);
	}
	SUBCASE("center at q/r/y-min, returns up, east, southeast") {
		check_neighbors(HexMapCellId(SHRT_MIN, SHRT_MIN, SHRT_MIN),
				HexMap::Planes::All, UP | EAST | SE);
	}
	SUBCASE("center at q/r/y-max, returns down, west, northwest") {
		check_neighbors(HexMapCellId(SHRT_MAX, SHRT_MAX, SHRT_MAX),
				HexMap::Planes::All, DOWN | WEST | NW);
	}
	SUBCASE("center at [0, 0, 32767]") {
		check_neighbors(HexMapCellId(0, 0, 32767), HexMap::Planes::All, ~UP);
	}
	SUBCASE("center at origin, plane QRS, excludes up and down") {
		check_neighbors(HexMapCellId(), HexMap::Planes::QRS, ~UP & ~DOWN);
	}
	SUBCASE("center at origin, plane YRS") {
		check_neighbors(
				HexMapCellId(), HexMap::Planes::YRS, UP | DOWN | NW | SE);
	}
	SUBCASE("center at origin, plane YQS") {
		check_neighbors(
				HexMapCellId(), HexMap::Planes::YQS, UP | DOWN | WEST | EAST);
	}
	SUBCASE("center at origin, plane YQR") {
		check_neighbors(
				HexMapCellId(), HexMap::Planes::YQR, UP | DOWN | SW | NE);
	}
}

TEST_CASE("HexMapIterAxial::begin() is within radius") {
	HexMapIterAxial iter(HexMapCellId(), 1);
	CHECK_MESSAGE((*iter.begin()).distance(HexMapCellId()) <= 1,
			"begin cell should be within the radius");
}
