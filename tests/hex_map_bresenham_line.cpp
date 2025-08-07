
#include "doctest.h"
#include "core/math.h"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "godot_cpp/templates/local_vector.hpp"

using namespace godot;

TEST_CASE("Bresenham Line Linear") {
    LocalVector<Point2i> truth {};
    LocalVector<Point2i> line {};

    // Vertical line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(0,1));
    truth.push_back(Point2i(0,2));
    
    line = bresenham_line(Point2i(0,0), Point2i(0,2));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Vertical line desc
    truth.push_back(Point2i(0,2));
    truth.push_back(Point2i(0,1));
    truth.push_back(Point2i(0,0));
    
    line = bresenham_line(Point2i(0,2), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Horizontal line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(1,0));
    truth.push_back(Point2i(2,0));
    
    line = bresenham_line(Point2i(0,0), Point2i(2,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Horizontal line desc
    truth.push_back(Point2i(2,0));
    truth.push_back(Point2i(1,0));
    truth.push_back(Point2i(0,0));
    
    line = bresenham_line(Point2i(2,0), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // 45 degree line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(1,1));
    truth.push_back(Point2i(2,2));
    
    line = bresenham_line(Point2i(0,0), Point2i(2,2));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // 45 degree line desc
    truth.push_back(Point2i(2,2));
    truth.push_back(Point2i(1,1));
    truth.push_back(Point2i(0,0));
    
    line = bresenham_line(Point2i(2,2), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // TODO: Write cases for less than and greater than 45 lines
}

TEST_CASE("Bresenham Line Hex") {
    LocalVector<Point2i> truth {};
    LocalVector<Point2i> line {};

    // Vertical line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(1,0));
    truth.push_back(Point2i(1,1));
    truth.push_back(Point2i(2,1));
    truth.push_back(Point2i(2,2));
    
    line = bresenham_line_hex(Point2i(0,0), Point2i(2,2));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Vertical line desc
    truth.push_back(Point2i(2,2));
    truth.push_back(Point2i(1,2));
    truth.push_back(Point2i(1,1));
    truth.push_back(Point2i(0,1));
    truth.push_back(Point2i(0,0));
    
    line = bresenham_line_hex(Point2i(2,2), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Horizontal line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(1,-1));
    truth.push_back(Point2i(2,-2));
    
    line = bresenham_line_hex(Point2i(0,0), Point2i(2,-2));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Horizontal line desc
    truth.push_back(Point2i(2,-2));
    truth.push_back(Point2i(1,-1));
    truth.push_back(Point2i(0,0));
    
    line = bresenham_line_hex(Point2i(2,-2), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // "Q" line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(1,0));
    truth.push_back(Point2i(2,0));

    line = bresenham_line_hex(Point2i(0,0), Point2i(2,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // "Q" line desc
    truth.push_back(Point2i(2,0));
    truth.push_back(Point2i(1,0));
    truth.push_back(Point2i(0,0));

    line = bresenham_line_hex(Point2i(2,0), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // "R" line asc
    truth.push_back(Point2i(0,0));
    truth.push_back(Point2i(0,1));
    truth.push_back(Point2i(0,2));

    line = bresenham_line_hex(Point2i(0,0), Point2i(0,2));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // "R" line desc
    truth.push_back(Point2i(0,2));
    truth.push_back(Point2i(0,1));
    truth.push_back(Point2i(0,0));

    line = bresenham_line_hex(Point2i(0,2), Point2i(0,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();

    // Non axial-line
    truth.push_back(Point2i(2,5));
    truth.push_back(Point2i(3,4));
    truth.push_back(Point2i(3,3));
    truth.push_back(Point2i(4,2));
    truth.push_back(Point2i(4,1));
    truth.push_back(Point2i(5,0));

    line = bresenham_line_hex(Point2i(2,5), Point2i(5,0));
    CHECK(truth.size() == line.size());
    for (size_t i = 0; i < truth.size(); i++)
    {
        CHECK_EQ(truth[i], line[i]);
    }
    truth.clear();
    line.clear();
}