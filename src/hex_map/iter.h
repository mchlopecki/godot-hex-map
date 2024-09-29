#pragma once

#include "godot_cpp/classes/ref.hpp"
#include "hex_map/cell_id.h"

using namespace godot;

class HexMapIterWrapper;

// generic interface for an iterator to be wrapped for gdscript
class HexMapIter {
public:
    HexMapIter() {}
    virtual ~HexMapIter() {}

    // ensure gdscript can print the iterator
    virtual operator godot::String() const = 0;

    // iteration support
    virtual bool _iter_init() = 0;
    virtual bool _iter_next() = 0;
    virtual HexMapCellId _iter_get() const = 0;

    // clone necessary to wrap for gdscript
    operator Ref<HexMapIterWrapper>() const;
    virtual HexMapIter *clone() const = 0;
};

class HexMapIterWrapper : public godot::RefCounted {
    using Inner = HexMapIter;
    using HexMapIter = HexMapIterWrapper;

    GDCLASS(HexMapIter, RefCounted);

public:
    HexMapIterWrapper(){};
    HexMapIterWrapper(const Inner &iter) : iter(iter.clone()){};
    ~HexMapIterWrapper() {
        if (iter != nullptr) {
            delete iter;
        }
    };

    // GDScript functions
    String _to_string() const;

    // GDScript custom iterator functions
    bool _iter_init(Variant _arg);
    bool _iter_next(Variant _arg);
    Ref<HexMapCellIdWrapper> _iter_get(Variant _arg);

protected:
    static void _bind_methods();

private:
    Inner *iter = nullptr;
};
