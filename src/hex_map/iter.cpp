#include "iter.h"

HexMapIter::operator Ref<HexMapIterWrapper>() const {
    return Ref<HexMapIterWrapper>(memnew(HexMapIterWrapper(*this)));
}

void HexMapIterWrapper::_bind_methods() {
    ClassDB::bind_method(
            D_METHOD("_iter_init", "_arg"), &HexMapIterWrapper::_iter_init);
    ClassDB::bind_method(
            D_METHOD("_iter_next", "_arg"), &HexMapIterWrapper::_iter_next);
    ClassDB::bind_method(
            D_METHOD("_iter_get", "_arg"), &HexMapIterWrapper::_iter_get);
}

// Godot custom iterator functions
bool HexMapIterWrapper::_iter_init(Variant _arg) { return iter->_iter_init(); }

bool HexMapIterWrapper::_iter_next(Variant _arg) { return iter->_iter_next(); }

Ref<HexMapCellIdWrapper> HexMapIterWrapper::_iter_get(Variant _arg) {
    return iter->_iter_get();
}

String HexMapIterWrapper::_to_string() const { return *iter; };
