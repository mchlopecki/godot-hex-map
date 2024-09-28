#pragma once

#include <climits>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/string.hpp>

#include "cell_id.h"
#include "hex_map.h"

class HexMapIterCubeRef;

// HexMap cell iterator using oddr coordinates
struct HexMapIterCube {
public:
    HexMapIterCube(Vector3 a, Vector3 b);

    inline operator Ref<HexMapIterCubeRef>() const;
    operator String() const;

    HexMapCellId operator*() const { return HexMapCellId::from_oddr(pos); }
    const Vector3i &operator->() { return pos; }
    HexMapIterCube &operator++();
    HexMapIterCube operator++(int);

    friend bool operator==(const HexMapIterCube &a, const HexMapIterCube &b) {
        return a.pos == b.pos;
    };
    friend bool operator!=(const HexMapIterCube &a, const HexMapIterCube &b) {
        return a.pos != b.pos;
    };

    HexMapIterCube begin() const;
    HexMapIterCube end() const;

    // added for testing
    friend std::ostream &operator<<(std::ostream &os,
            const HexMapIterCube &value);

private:
    Vector3i min;
    Vector3i max;
    Vector3i pos;
    int min_x_shift[2] = { 0, 0 };
    int max_x_shift[2] = { 0, 0 };
    int max_x;
};

// GDScript wrapper for HexMapIterCube
class HexMapIterCubeRef : public godot::RefCounted {
    GDCLASS(HexMapIterCubeRef, RefCounted);

public:
    HexMapIterCubeRef() : iter(HexMapIterCube(Vector3(), Vector3())){};
    HexMapIterCubeRef(const HexMapIterCube &iter) : iter(iter){};

    // GDScript functions
    String _to_string() const;

    // GDScript custom iterator functions
    bool _iter_init(Variant _arg);
    bool _iter_next(Variant _arg);
    Ref<HexMapCellIdWrapper> _iter_get(Variant _arg);

protected:
    static void _bind_methods();

private:
    HexMapIterCube iter;
};

inline HexMapIterCube::operator Ref<HexMapIterCubeRef>() const {
    return Ref<HexMapIterCubeRef>(memnew(HexMapIterCubeRef(*this)));
}
