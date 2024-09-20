#include <gdextension_interface.h>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "hex_map/editor/hex_map_editor_plugin.h"
#include "hex_map/hex_map.h"
#include "hex_map/hex_map_cell_id.h"
#include "hex_map/hex_map_iter_axial.h"
#include "test_node.h"
#include "test_node_editor_plugin.h"

using namespace godot;

void initialize_hexmap_module(ModuleInitializationLevel p_level) {
	if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<HexMapCellIdRef>();
		ClassDB::register_class<HexMapIterAxialRef>();
		ClassDB::register_class<HexMap>();
		ClassDB::register_class<TestNode>();
	}

#ifdef TOOLS_ENABLED
	if (p_level == godot::MODULE_INITIALIZATION_LEVEL_EDITOR) {
		ClassDB::register_internal_class<MeshLibraryPalette>();
		ClassDB::register_internal_class<EditorControl>();
		ClassDB::register_internal_class<HexMapEditorPlugin>();
		EditorPlugins::add_by_type<HexMapEditorPlugin>();

		ClassDB::register_internal_class<TestNodeEditorPlugin>();
		EditorPlugins::add_by_type<TestNodeEditorPlugin>();
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
