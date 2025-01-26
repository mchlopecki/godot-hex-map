#pragma once
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/shortcut.hpp>

#define ED_IS_SHORTCUT(name, event)                                           \
    ({                                                                        \
        Ref<Shortcut> shortcut = EditorInterface::get_singleton()             \
                                         ->get_editor_settings()              \
                                         ->get_setting(name);                 \
        shortcut.is_valid() && shortcut->matches_event(event);                \
    })
