#include "iter_axial.h"
#include "hex_map/cell_id.h"

void HexMapIterAxial::move_to_first_valid_cell() {
    cell = HexMapCellId(q_min, r_min, y_min);
    if (cell.s() < s_min || cell.s() > s_max || !cell.in_bounds()) {
        operator++();
    }
}
HexMapIterAxial::HexMapIterAxial(const HexMapCellId center,
        unsigned int radius,
        const HexMap::Planes &planes) :
        center(center) {
    if (planes.y) {
        y_min = center.y - radius;
        y_max = center.y + radius;
    } else {
        y_min = y_max = center.y;
    }
    // XXX prune min/max to only include valid space

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
    move_to_first_valid_cell();
}

// prefix increment
HexMapIterAxial &HexMapIterAxial::operator++() {
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
    } while (cell.s() < s_min || cell.s() > s_max || !cell.in_bounds());
    return *this;
}

// postfix increment
HexMapIterAxial HexMapIterAxial::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

HexMapIterAxial HexMapIterAxial::begin() {
    HexMapIterAxial iter = *this;
    iter.move_to_first_valid_cell();
    return iter;
}

HexMapIterAxial HexMapIterAxial::end() {
    HexMapIterAxial iter = *this;
    iter.cell = HexMapCellId::Invalid;
    return iter;
}

HexMapIterAxial::operator String() const {
    // clang-format off
    return (String)"{ " +
        ".center = " + center.operator String() + ", " +
        ".cell = " + cell.operator String() + ", " +
        ".q = [" + itos(q_min) + ".." + itos(q_max) + "], " +
        ".r = [" + itos(r_min) + ".." + itos(r_max) + "], " +
        ".s = [" + itos(s_min) + ".." + itos(s_max) + "], " +
        ".y = [" + itos(y_min) + ".." + itos(y_max) + "], " +
    "}";
    // clang-format on
}

void HexMapIterAxialWrapper::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_iter_init", "_arg"),
            &HexMapIterAxialWrapper::_iter_init);
    ClassDB::bind_method(D_METHOD("_iter_next", "_arg"),
            &HexMapIterAxialWrapper::_iter_next);
    ClassDB::bind_method(
            D_METHOD("_iter_get", "_arg"), &HexMapIterAxialWrapper::_iter_get);
}

// Godot custom iterator functions
bool HexMapIterAxialWrapper::_iter_init(Variant _arg) {
    iter = iter.begin();
    return *iter != HexMapCellId::Invalid;
}

bool HexMapIterAxialWrapper::_iter_next(Variant _arg) {
    ++iter;
    return *iter != HexMapCellId::Invalid;
}

Ref<HexMapCellIdWrapper> HexMapIterAxialWrapper::_iter_get(Variant _arg) {
    return *iter;
}

String HexMapIterAxialWrapper::_to_string() const { return iter; };
