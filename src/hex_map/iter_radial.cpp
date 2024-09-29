#include "iter_radial.h"

HexMapIterRadial::HexMapIterRadial(const HexMapCellId center,
        unsigned int radius,
        const HexMap::Planes &planes) :
        center(center), radius(radius) {
    if (!center.in_bounds()) {
        radius = 0;
    }

    if (planes.y) {
        y_min = center.y - radius;
        y_max = center.y + radius;
    } else {
        y_min = y_max = center.y;
    }
    if (planes.q) {
        q_min = center.q - radius;
        q_max = center.q + radius;
    } else {
        q_min = q_max = center.q;
    }
    if (planes.r) {
        r_min = center.r - radius;
        r_max = center.r + radius;
    } else {
        r_min = r_max = center.r;
    }
    if (planes.s) {
        s_min = center.s() - radius;
        s_max = center.s() + radius;
    } else {
        s_min = s_max = center.s();
    }

    cell = HexMapCellId(q_min, r_min, y_min);
    if (center.distance(cell) > radius) {
        operator++();
    }
}

// prefix increment
HexMapIterRadial &HexMapIterRadial::operator++() {
    do {
        if (cell.r < r_max) {
            cell.r++;
        } else if (cell.q < q_max) {
            cell.r = r_min;
            cell.q++;
        } else if (cell.y < y_max) {
            cell.r = r_min;
            cell.q = q_min;
            cell.y++;
        } else {
            cell = HexMapCellId::Invalid;
            break;
        }
    } while (cell.s() < s_min || cell.s() > s_max || cell == center ||
            !cell.in_bounds() || center.distance(cell) > radius);
    return *this;
}

// postfix increment
HexMapIterRadial HexMapIterRadial::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HexMapIterRadial HexMapIterRadial::begin() {
    HexMapIterRadial iter = *this;
    iter.cell = HexMapCellId(q_min, r_min, y_min);
    ++iter;
    return iter;
}

HexMapIterRadial HexMapIterRadial::end() {
    HexMapIterRadial iter = *this;
    iter.cell = HexMapCellId::Invalid;
    return iter;
}

HexMapIterRadial::operator String() const {
    // clang-format off
    return (String)"{ " +
        ".center = " + center.operator String() + ", " +
        ".radius = " + itos(radius) + ", " +
        ".cell = " + cell.operator String() + ", " +
        ".q = [" + itos(q_min) + ".." + itos(q_max) + "], " +
        ".r = [" + itos(r_min) + ".." + itos(r_max) + "], " +
        ".s = [" + itos(s_min) + ".." + itos(s_max) + "], " +
        ".y = [" + itos(y_min) + ".." + itos(y_max) + "], " +
    "}";
    // clang-format on
}

void HexMapIterRadialWrapper::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_iter_init", "_arg"),
            &HexMapIterRadialWrapper::_iter_init);
    ClassDB::bind_method(D_METHOD("_iter_next", "_arg"),
            &HexMapIterRadialWrapper::_iter_next);
    ClassDB::bind_method(D_METHOD("_iter_get", "_arg"),
            &HexMapIterRadialWrapper::_iter_get);
}

// Godot custom iterator functions
bool HexMapIterRadialWrapper::_iter_init(Variant _arg) {
    iter = iter.begin();
    return *iter != HexMapCellId::Invalid;
}

bool HexMapIterRadialWrapper::_iter_next(Variant _arg) {
    ++iter;
    return *iter != HexMapCellId::Invalid;
}

Ref<HexMapCellIdWrapper> HexMapIterRadialWrapper::_iter_get(Variant _arg) {
    return *iter;
}

String HexMapIterRadialWrapper::_to_string() const { return iter; };
