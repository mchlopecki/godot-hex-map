#include "formatters.h"
#include <ostream>

namespace godot {
std::ostream &operator<<(std::ostream &os, const Vector3 &value) {
    // clang-format off
		os << "{ .x = " << value.x
		   << ", .y = " << value.y
		   << ", .z = " << value.z << "}";
    // clang-format on
    return os;
}

std::ostream &operator<<(std::ostream &os, const Vector3i &value) {
    // clang-format off
		os << "{ .x = " << value.x
		   << ", .y = " << value.y
		   << ", .z = " << value.z << "}";
    // clang-format on
    return os;
}
} //namespace godot

std::ostream &operator<<(std::ostream &os, const int value[2]) {
    os << "{ " << value[0] << ", " << value[1] << " }";
    return os;
}
std::ostream &operator<<(std::ostream &os, const HexMapIterCube &value) {
    // clang-format off
    os << "{ .min = " << value.min
       << "(" << HexMapCellId::from_oddr(value.min) << ")"
       << ", .max = " << value.max
       << "(" << HexMapCellId::from_oddr(value.max) << ")"
       << ", .pos = " << value.pos
       << "(" << HexMapCellId::from_oddr(value.pos) << ")"
       << ", .min_x_shift = " << value.min_x_shift
       << ", .max_x_shift = " << value.max_x_shift
       << "}";
    // clang-format on
    return os;
}

std::ostream &operator<<(std::ostream &os,
        const std::set<HexMapCellId> &value) {
    os << "{ ";
    for (const HexMapCellId &cell : value) {
        os << cell << ", ";
    }
    os << "}";

    return os;
}

std::ostream &operator<<(std::ostream &os,
        const std::vector<HexMapCellId> &value) {
    os << "{ ";
    for (const HexMapCellId &cell : value) {
        os << cell << ", ";
    }
    os << "}";

    return os;
}
