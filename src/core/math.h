#pragma once

#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/templates/local_vector.hpp>

// SQRT(3)/2; used both in the editor and the GridMap.  Due to the division, it
// didn't fit the pattern of other Math_SQRTN defines, so I'm putting it here.
#define Math_SQRT3 1.7320508075688772935274463415059
#define Math_SQRT3_2 0.8660254037844386

using namespace godot;

LocalVector<Point2i> bresenham_line(Point2i from, Point2i to);
LocalVector<Point2i> bresenham_line_hex(Point2i from, Point2i to);
