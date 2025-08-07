#include "math.h"
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

// CREDIT: Godot's implementation 
// (https://github.com/godotengine/godot/blob/006e3f26b9cf1867558ada7ea68c9eacb9d57b08/core/math/geometry_2d.h#L461)
LocalVector<Point2i> bresenham_line(Point2i from, Point2i to) {
    LocalVector<Point2i> points {};

    Vector2i delta = (to - from).abs() * 2;
    Vector2i step = (to - from).sign();
    Point2i current = from;
    int err;
    
    if (delta.x > delta.y) {
        err = delta.x / 2;
        for (; current.x != to.x; current.x += step.x) {
            points.push_back(current);

            err -= delta.y;
            if (err < 0) {
                current.y += step.y;
                err += delta.x;
            }
        }
    } else /* delta.y >= delta.x */ {
        err = delta.y / 2;
        for (; current.y != to.y; current.y += step.y) {
            points.push_back(current);

            err -= delta.x;
            if (err < 0) {
                current.x += step.x;
                err += delta.y;
            }
        }
    }

    points.push_back(current);
    return points;
}

// Because the godot implementation assumes a different hexagon layout, 
// we instead use the credited article which easily adapts to our needs
// CREDIT: http://www-cs-students.stanford.edu/~amitp/Articles/HexLOS.html
LocalVector<Point2i> bresenham_line_hex(Point2i from, Point2i to) {
    LocalVector<Point2i> points {};

    Vector2i delta = to - from;
    Vector2i step = delta.sign();
    bool sig = (bool)(step.x + 1) != (bool)(step.y + 1);
    delta = delta.abs() * 2;
    Point2i current = from;
    points.push_back(current);

    if (delta.x >= delta.y) {
        int err = delta.x / 2;
        while (current.x != to.x || current.y != to.y) {
            err += delta.y;
            if (err >= delta.x) {
                err -= delta.x;
                if (sig) {
                    current.x += step.x;
                    current.y += step.y;
                } else {
                    current.x += step.x;
                    points.push_back(current);
                    current.y += step.y;
                }
            } else {
                current.x += step.x;
            }
            points.push_back(current);
        }
    } else {
        int err = delta.y / 2;
        while (current.x != to.x || current.y != to.y) {
            err += delta.x;
            if (err >= delta.y) {
                err -= delta.y;
                if (sig) {
                    current.x += step.x;
                    current.y += step.y;
                } else {
                    current.y += step.y;
                    points.push_back(current);
                    current.x += step.x;
                }
            } else {
                current.y += step.y;
            }
            points.push_back(current);
        }
    }
    return points;
}