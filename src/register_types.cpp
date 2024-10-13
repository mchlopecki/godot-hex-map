#include <gdextension_interface.h>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "core/cell_id.h"
#include "core/hex_map_base.h"
#include "core/iter.h"
#include "hex_map/editor/hex_map_editor_plugin.h"
#include "hex_map/hex_map.h"

using namespace godot;

void initialize_hexmap_module(ModuleInitializationLevel p_level) {
    if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        ClassDB::register_class<HexMapCellIdWrapper>();
        ClassDB::register_class<HexMapIterWrapper>();
        ClassDB::register_abstract_class<HexMapBase>();
        ClassDB::register_class<HexMap>();
    }

#ifdef TOOLS_ENABLED
    if (p_level == godot::MODULE_INITIALIZATION_LEVEL_EDITOR) {
        ClassDB::register_internal_class<EditorControl>();
        ClassDB::register_internal_class<HexMapEditorPlugin>();
        EditorPlugins::add_by_type<HexMapEditorPlugin>();
    }
#endif
}

void uninitialize_hexmap_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT hexmap_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_hexmap_module);
    init_obj.register_terminator(uninitialize_hexmap_module);
    init_obj.set_minimum_library_initialization_level(
            MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
