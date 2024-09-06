/**************************************************************************/
/*  grid_map_editor_plugin.cpp                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "hex_map_editor_plugin.h"
#include "editor_cursor.h"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "grid_manager.h"

#include <cassert>
#include <cstdint>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// TTR not defined for gdext
#define TTR(s) s
#define SNAME(s) s
#define EditorStringName(s) #s
#define SceneStringName(s) #s

#define EDSCALE                                                               \
	(float)godot::ProjectSettings::get_singleton()->get_setting(              \
			"display/window/stretch/scale")

#define ED_GET_SHORTCUT(n)                                                    \
	get_editor_interface()->get_editor_settings()->get_setting(n)
#define EDITOR_GET(n)                                                         \
	get_editor_interface()->get_editor_settings()->get_setting(n)
#define ED_SHORTCUT(p, n, k, phys)                                            \
	({                                                                        \
		Array __events;                                                       \
		Ref<InputEventKey> __ie;                                              \
		__ie->set_physical_keycode(                                           \
				(Key)(k & KeyModifierMask::KEY_CODE_MASK));                   \
		__events.push_back(__ie);                                             \
		if (ProjectSettings *__ps = ProjectSettings::get_singleton()) {       \
			Ref<Shortcut> __sc = __ps->get_setting(p);                        \
		}                                                                     \
	})

void HexMapEditorPlugin::_configure() {
	if (!hex_map) {
		return;
	}

	update_grid();
}

void HexMapEditorPlugin::_update_selection() {
	deselect_tiles();
	selection.active = true;

	RenderingServer *rs = RS::get_singleton();

	// Scaling and translation for the center of the cell mesh.
	Vector3 cell_center = Vector3(
			hex_map->get_center_x() ? 0 : hex_map->get_cell_size().x / 2.0,
			hex_map->get_center_y() ? 0 : hex_map->get_cell_size().y / 2.0,
			hex_map->get_center_z() ? 0 : hex_map->get_cell_size().z / 2.0);
	Transform3D cell_transform =
			Transform3D()
					.scaled_local(hex_map->get_cell_size())
					.translated(cell_center);

	// We're using `local_region_to_map()` to get a selection of cells, and
	// that function returns cells within an axis-aligned bounding box.  The Q
	// and S axis are not axis-aligned (they run diagonal along X/Z, so we'll
	// need to filter the results from `local_region_map()` to make sure they
	// all fall along a plane of the edit axis.  To do this, we'll need to use
	// the cell index of the beginning point.
	Vector3i begin = hex_map->local_to_map(selection.begin);

	// Get the cells using our selection begin & end points to define an axis-
	// aligned region of cells.
	TypedArray<Vector3i> cells =
			hex_map->local_region_to_map(selection.begin, selection.end);

	// Add the cells to our selection multimesh
	rs->multimesh_allocate_data(
			selection_multimesh, cells.size(), RS::MULTIMESH_TRANSFORM_3D);
	for (int i = 0; i < cells.size(); i++) {
		Vector3i cell = cells[i];
		switch (edit_axis) {
			case EditorControl::AXIS_Q:
				// We're using knowledge of the internal coordinate system of
				// hex cells in the GridMap here, which makes this brittle to
				// change,  The axial Q axis value is stored in the X field.
				// So exclude any cell that doesn't have the same X value
				if (cell.x != begin.x) {
					continue;
				}
				break;
			case EditorControl::AXIS_S:
				// The S axis value can be calculated from the Q/R values
				// stored in X & Z.  If the S value of the cell doesn't match
				// that of the beginning cell, the cell doesn't fall on the
				// same S-axis plane, so we can exclude it from the selection.
				if (-cell.x - cell.z != -begin.x - begin.z) {
					continue;
				}
				break;

			default:
				break;
		}
		selection.cells.append(cell);
		rs->multimesh_instance_set_transform(selection_multimesh, i,
				cell_transform.translated(hex_map->map_to_local(cell)));
	}

	// create an instance of the multimesh with the transform of our node
	selection_multimesh_instance = rs->instance_create2(selection_multimesh,
			get_tree()->get_root()->get_world_3d()->get_scenario());
	rs->instance_set_transform(
			selection_multimesh_instance, hex_map->get_global_transform());
	rs->instance_set_layer_mask(selection_multimesh_instance,
			/* Node3DEditorViewport::MISC_TOOL_LAYER */ 1 << 24);
}

void HexMapEditorPlugin::_set_selection(
		bool p_active, const Vector3 &p_begin, const Vector3 &p_end) {
	selection.active = p_active;
	selection.begin = p_begin;
	selection.end = p_end;

	if (is_inside_tree()) {
		_update_selection();
	}
	// XXX split _set_selection out into set & clear selection
	editor_control->update_selection_menu(true);

	// XXX missing visibility check
	// if (is_visible_in_tree()) {
	// 	_update_selection();
	// }
}

void HexMapEditorPlugin::deselect_tiles() {
	RenderingServer *rs = RS::get_singleton();
	if (selection_multimesh_instance.is_valid()) {
		rs->free_rid(selection_multimesh_instance);
	}
	selection.cells.clear();
	selection.active = false;
	editor_control->update_selection_menu(false);
}

bool HexMapEditorPlugin::do_input_action(
		Camera3D *p_camera, const Point2 &p_point, bool p_click) {
	Camera3D *camera = p_camera;
	Vector3 from = camera->project_ray_origin(p_point);
	Vector3 normal = camera->project_ray_normal(p_point);
	Transform3D local_xform = hex_map->get_global_transform().affine_inverse();
	TypedArray<Plane> planes = camera->get_frustum();
	from = local_xform.xform(from);
	normal = local_xform.basis.xform(normal).normalized();

	Vector3 edit_plane_point;

	// XXX need pick distance in EditorControl
	if (!edit_plane.intersects_segment(
				from, from + normal * 5000, &edit_plane_point)) {
		return false;
	}

	// Make sure the intersection is inside the frustum planes, to avoid
	// Painting on invisible regions.
	for (int i = 0; i < planes.size(); i++) {
		Plane fp = local_xform.xform((Plane)planes[i]);
		if (fp.is_point_over(edit_plane_point)) {
			return false;
		}
	}

	pointer_cell = hex_map->local_to_map(edit_plane_point);
	if (editor_cursor) {
		editor_cursor->set_pointer(pointer_cell);
		grid_manager->set_center(HexMapCellId(pointer_cell));
	}

	if (input_action == INPUT_PASTE) {
		// _update_paste_indicator();

	} else if (input_action == INPUT_SELECT) {
		if (p_click) {
			selection.begin = edit_plane_point;
		}
		selection.end = edit_plane_point;
		selection.active = true;
		_update_selection();
		editor_control->update_selection_menu(true);

		return true;
	} else if (input_action == INPUT_PICK) {
		// XXX allow picking from tiles on map,
		int item = hex_map->get_cell_item(pointer_cell);
		if (item >= 0) {
			// XXX should send a signal back when called; so we probably don't
			// need to set it ourselves here.
			mesh_palette->set_mesh(item);

			// _update_cursor_instance();
		}
		return true;
	}

	if (input_action == INPUT_PAINT) {
		List<EditorCursor::CursorCell> tiles = editor_cursor->get_tiles();
		if (tiles.is_empty()) {
			return true;
		}
		EditorCursor::CursorCell &tile = tiles[0];
		// XXX disallow painting outside of selection box if selected
		SetItem si;
		si.position = pointer_cell;
		si.new_value = tile.tile;
		si.new_orientation = tile.orientation;
		si.old_value = hex_map->get_cell_item(pointer_cell);
		si.old_orientation = hex_map->get_cell_item_orientation(pointer_cell);
		set_items.push_back(si);
		hex_map->set_cell_item(pointer_cell, tile.tile, tile.orientation);
		return true;
	} else if (input_action == INPUT_ERASE) {
		SetItem si;
		si.position =
				Vector3i(pointer_cell[0], pointer_cell[1], pointer_cell[2]);
		si.new_value = -1;
		si.new_orientation = 0;
		si.old_value = hex_map->get_cell_item(pointer_cell);
		si.old_orientation = hex_map->get_cell_item_orientation(pointer_cell);
		set_items.push_back(si);
		hex_map->set_cell_item(pointer_cell, -1);
		return true;
	}

	return false;
}

void HexMapEditorPlugin::selection_clear() {
	if (!selection.active) {
		return;
	}

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("GridMap Delete Selection"));

	for (const Vector3i cell : selection.cells) {
		undo_redo->add_do_method(
				hex_map, "set_cell_item", cell, HexMap::INVALID_CELL_ITEM);
		undo_redo->add_undo_method(hex_map, "set_cell_item", cell,
				hex_map->get_cell_item(cell),
				hex_map->get_cell_item_orientation(cell));
	}

	undo_redo->add_do_method(
			this, "_set_selection", false, selection.begin, selection.end);
	undo_redo->add_undo_method(
			this, "_set_selection", true, selection.begin, selection.end);
	undo_redo->commit_action();
}

void HexMapEditorPlugin::selection_fill() {
	if (!selection.active) {
		return;
	}

	List<EditorCursor::CursorCell> tiles = editor_cursor->get_tiles();
	ERR_FAIL_COND_MSG(
			tiles.size() > 1, "cursor has more than one tile; need one");
	ERR_FAIL_COND_MSG(tiles.size() == 0, "cursor has no tiles; need one");
	EditorCursor::CursorCell &tile = tiles[0];

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("GridMap Fill Selection"));

	for (const Vector3i cell : selection.cells) {
		undo_redo->add_do_method(
				hex_map, "set_cell_item", cell, tile.tile, tile.orientation);
		undo_redo->add_undo_method(hex_map, "set_cell_item", cell,
				hex_map->get_cell_item(cell),
				hex_map->get_cell_item_orientation(cell));
	}

	undo_redo->add_do_method(
			this, "_set_selection", false, selection.begin, selection.end);
	undo_redo->add_undo_method(
			this, "_set_selection", true, selection.begin, selection.end);
	undo_redo->commit_action();
}

void HexMapEditorPlugin::selection_begin_move() {
	ERR_FAIL_COND(editor_cursor == nullptr);

	// make the current pointer cell the origin cell in the cursor.

	editor_cursor->clear_tiles();
	editor_control->reset_orientation();

	if (selection.cells.size() == 0) {
		return;
	}

	for (const Vector3i cell : selection.cells) {
		int cell_item = hex_map->get_cell_item(cell);
		if (cell_item == HexMap::INVALID_CELL_ITEM) {
			continue;
		}
		editor_cursor->set_tile(cell - pointer_cell, cell_item,
				hex_map->get_cell_item_orientation(cell));
	}
	editor_cursor->redraw();

	input_action = INPUT_PASTE;
}

void HexMapEditorPlugin::selection_clone() {
	// XXX need to distinguish clone from move
	// when move completes, save the clipboard to allow quick duplicate
	selection_begin_move();
	deselect_tiles();
}

void HexMapEditorPlugin::selection_move() {
	// XXX need to distinguish move from clone
	// when escape is pressed, need to undo clear selection/ret clipboard
	// contents.
	selection_begin_move();
	// XXX need to be able to undo this
	selection_clear();
	deselect_tiles();
}

void HexMapEditorPlugin::_do_paste() {
	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("HexMap Move Selection"));

	for (const auto &cell : editor_cursor->get_tiles()) {
		undo_redo->add_do_method(hex_map, "set_cell_item", cell.cell_id_live,
				cell.tile, cell.orientation);
		undo_redo->add_undo_method(hex_map, "set_cell_item", cell.cell_id_live,
				hex_map->get_cell_item(cell.cell_id_live),
				hex_map->get_cell_item_orientation(cell.cell_id_live));
	}
	undo_redo->commit_action();

	// XXX save off tiles so we can allow user to hit G again to immediately
	// move that last selection again

	editor_control->reset_orientation();
	editor_cursor->clear_tiles();
	editor_cursor->set_tile(Vector3(0, 0, 0), mesh_palette->get_mesh());
	editor_cursor->redraw();
}

bool HexMapEditorPlugin::_handles(Object *p_object) const {
	return p_object->is_class("HexMap");
}

void HexMapEditorPlugin::_make_visible(bool p_visible) {
	if (p_visible) {
		editor_control->show();
		mesh_palette->show();
		set_process(true);
	} else {
		editor_control->hide();
		mesh_palette->hide();
		set_process(false);
	}
}

int32_t HexMapEditorPlugin::_forward_3d_gui_input(
		Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!hex_map) {
		return EditorPlugin::AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		// XXX allow floor change with mouse-based control
		// if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_UP &&
		// (mb->is_command_or_control_pressed())) { 	if (mb->is_pressed()) {
		// 		floor->set_value(floor->get_value() + mb->get_factor());
		// 	}
		//
		// 	return EditorPlugin::AFTER_GUI_INPUT_STOP; // Eaten.
		// } else if (mb->get_button_index() ==
		// MouseButton::MOUSE_BUTTON_WHEEL_DOWN &&
		// (mb->is_command_or_control_pressed())) { 	if (mb->is_pressed()) {
		// 		floor->set_value(floor->get_value() - mb->get_factor());
		// 	}
		// 	return EditorPlugin::AFTER_GUI_INPUT_STOP;
		// }

		if (mb->is_pressed()) {
			// Node3DEditorViewport::NavigationScheme nav_scheme =
			// (Node3DEditorViewport::NavigationScheme)EDITOR_GET("editors/3d/navigation/navigation_scheme").operator
			// int(); if ((nav_scheme == Node3DEditorViewport::NAVIGATION_MAYA
			// || nav_scheme == Node3DEditorViewport::NAVIGATION_MODO) &&
			// mb->is_alt_pressed()) { 	input_action = INPUT_NONE;
			if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
				bool can_edit =
						(hex_map && hex_map->get_mesh_library().is_valid());
				if (input_action == INPUT_PASTE) {
					_do_paste();
					input_action = INPUT_NONE;
					return EditorPlugin::AFTER_GUI_INPUT_STOP;
				} else if (mb->is_shift_pressed() && can_edit) {
					input_action = INPUT_SELECT;
					last_selection = selection;
					editor_cursor->hide();
				} else if (mb->is_command_or_control_pressed() && can_edit) {
					input_action = INPUT_PICK;
				} else {
					input_action = INPUT_PAINT;
					set_items.clear();
				}
			} else if (mb->get_button_index() ==
					MouseButton::MOUSE_BUTTON_RIGHT) {
				if (input_action == INPUT_PASTE) {
					input_action = INPUT_NONE;
					return EditorPlugin::AFTER_GUI_INPUT_STOP;
				} else if (selection.active) {
					deselect_tiles();
					return EditorPlugin::AFTER_GUI_INPUT_STOP;
				} else {
					input_action = INPUT_ERASE;
					set_items.clear();
				}
			} else {
				return EditorPlugin::AFTER_GUI_INPUT_PASS;
			}

			if (do_input_action(p_camera,
						Point2(mb->get_position().x, mb->get_position().y),
						true)) {
				return EditorPlugin::AFTER_GUI_INPUT_STOP;
			}
			return EditorPlugin::AFTER_GUI_INPUT_PASS;
		} else {
			if ((mb->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT &&
						input_action == INPUT_ERASE) ||
					(mb->get_button_index() ==
									MouseButton::MOUSE_BUTTON_LEFT &&
							input_action == INPUT_PAINT)) {
				if (set_items.size()) {
					EditorUndoRedoManager *undo_redo = get_undo_redo();
					undo_redo->create_action(TTR("GridMap Paint"));
					for (const SetItem &si : set_items) {
						undo_redo->add_do_method(hex_map, "set_cell_item",
								si.position, si.new_value, si.new_orientation);
					}
					for (List<SetItem>::Element *E = set_items.back(); E;
							E = E->prev()) {
						const SetItem &si = E->get();
						undo_redo->add_undo_method(hex_map, "set_cell_item",
								si.position, si.old_value, si.old_orientation);
					}

					undo_redo->commit_action();
				}
				set_items.clear();
				input_action = INPUT_NONE;

				if (set_items.size() > 0) {
					return EditorPlugin::AFTER_GUI_INPUT_STOP;
				}
				return EditorPlugin::AFTER_GUI_INPUT_PASS;
			}

			if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT &&
					input_action == INPUT_SELECT) {
				EditorUndoRedoManager *undo_redo = get_undo_redo();
				undo_redo->create_action(TTR("GridMap Selection"));
				undo_redo->add_do_method(this, "_set_selection",
						selection.active, selection.begin, selection.end);
				undo_redo->add_undo_method(this, "_set_selection",
						last_selection.active, last_selection.begin,
						last_selection.end);
				undo_redo->commit_action();
				editor_cursor->show();
			}

			if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT &&
					input_action != INPUT_NONE) {
				set_items.clear();
				input_action = INPUT_NONE;
				return EditorPlugin::AFTER_GUI_INPUT_STOP;
			}
			if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT &&
					(input_action == INPUT_ERASE ||
							input_action == INPUT_PASTE)) {
				input_action = INPUT_NONE;
				return EditorPlugin::AFTER_GUI_INPUT_STOP;
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		if (do_input_action(p_camera, mm->get_position(), false)) {
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		}
		return EditorPlugin::AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventKey> k = p_event;

	if (k.is_valid() && k->is_pressed()) {
		if (k->get_keycode() == Key::KEY_ESCAPE) {
			if (input_action == INPUT_PASTE) {
				input_action = INPUT_NONE;
				editor_cursor->clear_tiles();
				return EditorPlugin::AFTER_GUI_INPUT_STOP;
			} else {
				editor_control->reset_orientation();
				editor_cursor->clear_tiles();
				mesh_palette->clear_selection();
				// _update_cursor_instance();
				return EditorPlugin::AFTER_GUI_INPUT_STOP;
			}
		}

		// pass other non-repeat keypresses to EditorControl to match shortcuts
		// XXX consider also blocking other keys here to prevent changing to
		// scale mode, or transform
		if (!k->is_echo() && editor_control->handle_keypress(k)) {
			editor_control->accept_event();
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		}
		return EditorPlugin::AFTER_GUI_INPUT_STOP;
	}

	Ref<InputEventPanGesture> pan_gesture = p_event;
	if (pan_gesture.is_valid()) {
		if (pan_gesture->is_alt_pressed() &&
				pan_gesture->is_command_or_control_pressed()) {
			const real_t delta = pan_gesture->get_delta().y * 0.5;
			accumulated_floor_delta += delta;
			int step = 0;
			if (ABS(accumulated_floor_delta) > 1.0) {
				step = SIGN(accumulated_floor_delta);
				accumulated_floor_delta -= step;
			}

			if (step) {
				// XXX set floor in EditorControl
				// floor->set_value(floor->get_value() + step);
			}
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		}
	}
	accumulated_floor_delta = 0.0;

	return EditorPlugin::AFTER_GUI_INPUT_PASS;
}

struct _CGMEItemSort {
	String name;
	int id = 0;
	_FORCE_INLINE_ bool operator<(const _CGMEItemSort &r_it) const {
		return name < r_it.name;
	}
};

void HexMapEditorPlugin::_update_mesh_library() {
	ERR_FAIL_NULL(hex_map);

	Ref<MeshLibrary> new_mesh_library = hex_map->get_mesh_library();
	if (new_mesh_library == mesh_library) {
		return;
	}

	if (mesh_library.is_valid()) {
		mesh_library->disconnect("changed",
				callable_mp(
						mesh_palette, &MeshLibraryPalette::set_mesh_library));
	}
	mesh_library = new_mesh_library;
	mesh_palette->set_mesh_library(mesh_library);

	if (mesh_library.is_valid()) {
		mesh_library->connect("changed",
				callable_mp(
						mesh_palette, &MeshLibraryPalette::set_mesh_library));
	}

	if (editor_cursor) {
		editor_control->reset_orientation();
		editor_cursor->clear_tiles();
	}

	// Update the cursor and grid in case the library is changed or removed.
	// _update_cursor_instance();
	update_grid();
}

void HexMapEditorPlugin::_edit(Object *p_object) {
	if (hex_map) {
		// hex_map->disconnect(SNAME("cell_size_changed"),
		// 		callable_mp(this, &HexMapEditorPlugin::_draw_grids));
		hex_map->disconnect("changed",
				callable_mp(this, &HexMapEditorPlugin::_update_mesh_library));
		if (mesh_library.is_valid()) {
			mesh_library = Ref<MeshLibrary>();
		}

		// we use delete here because EditorCursor is not a Godot Object
		// subclass, so its destructor is not called with memfree().
		delete editor_cursor;
		editor_cursor = nullptr;

		delete grid_manager;
		grid_manager = nullptr;
	}

	// XXX works when no connect() calls made in constructor; waiting on 4.3
	// which will have https://github.com/godotengine/godot-cpp/pull/1446
	hex_map = Object::cast_to<HexMap>(p_object);

	input_action = INPUT_NONE;
	selection.active = false;
	_build_selection_meshes();
	deselect_tiles();
	mesh_palette->clear_selection();

	// spatial_editor =
	// Object::cast_to<Node3DEditorPlugin>(EditorNode::get_singleton()->get_editor_plugin_screen());

	if (!hex_map) {
		set_process(false);
		for (int i = 0; i < 3; i++) {
			RenderingServer::get_singleton()->instance_set_visible(
					grid_instance[i], false);
		}

		return;
	}

	// not a godot Object subclass, so `new` instead of `memnew()`
	editor_cursor = new EditorCursor(hex_map);

	// create a new grid manager and update it for the current depth
	grid_manager = new GridManager(hex_map);
	grid_manager->set_axis(editor_control->get_active_axis());
	grid_manager->set_depth(editor_control->get_plane());
	grid_manager->show();

	// _update_cursor_instance();

	set_process(true);

	// XXX Save the editor floor state
	//
	// load any previous floor values
	// TypedArray<int> floors = node->get_meta("_editor_floor_",
	// TypedArray<int>()); for (int i = 0; i < MIN(floors.size(), AXIS_MAX);
	// i++) { 	edit_floor[i] = floors[i];
	// }
	//
	// _draw_grids(hex_map->get_cell_size());
	// update_grid();

	// hex_map->connect(SNAME("cell_size_changed"),
	// 		callable_mp(this, &HexMapEditorPlugin::_draw_grids));
	hex_map->connect("changed",
			callable_mp(this, &HexMapEditorPlugin::_update_mesh_library));
	_update_mesh_library();
}

// update the grid mesh displayed in the editor
void HexMapEditorPlugin::update_grid() {
	RenderingServer *rs = RS::get_singleton();
	Vector3 cell_size = hex_map->get_cell_size();

	// Hex planes Q, R, and S need to offset the grid by half a cell on even
	// numbered floors.  We calculate this value here to simplify the code
	// later.
	int is_even_floor = (edit_depth & 1) == 0;

	// hide the active grid
	if (active_grid_instance.is_valid()) {
		rs->instance_set_visible(active_grid_instance, false);
		active_grid_instance = RID();
	}

	real_t cell_depth;
	Transform3D grid_transform;
	Menu menu_axis;

	// switch the edit plane and pick the new active grid and rotate if
	// necessary
	switch (edit_axis) {
		case EditorControl::AXIS_X:
			// set which grid to display
			active_grid_instance = grid_instance[0];
			// set the edit plane normal, and cell depth (used by the plane)
			edit_plane.normal = Vector3(1, 0, 0);
			cell_depth = SQRT3_2 * cell_size.x;
			// shift the edit grid based on which floor we are on
			if (!is_even_floor) {
				grid_transform.translate_local(
						Vector3(0, 0, 1.5 * cell_size.x));
			}
			// update the menu
			menu_axis = MENU_OPTION_X_AXIS;
			break;
		case EditorControl::AXIS_Y:
			active_grid_instance = grid_instance[1];
			edit_plane.normal = Vector3(0, 1, 0);
			cell_depth = cell_size.y;
			menu_axis = MENU_OPTION_Y_AXIS;
			break;
		case EditorControl::AXIS_Q: // hex plane, northwest to southeast
			active_grid_instance = grid_instance[2];
			edit_plane.normal = Vector3(SQRT3_2, 0, -0.5).normalized();
			cell_depth = 1.5 * cell_size.x;
			grid_transform.rotate(Vector3(0, 1, 0), -Math_PI / 3.0);
			// offset the edit grid on even numbered floors by half a cell
			grid_transform.translate_local(
					Vector3(is_even_floor * SQRT3_2 * cell_size.x, 0, 0));
			menu_axis = MENU_OPTION_Q_AXIS;
			break;
		case EditorControl::AXIS_R:
			active_grid_instance = grid_instance[2];
			edit_plane.normal = Vector3(0, 0, 1);
			cell_depth = 1.5 * cell_size.x;
			grid_transform.translate_local(
					Vector3(is_even_floor * SQRT3_2 * cell_size.x, 0, 0));
			menu_axis = MENU_OPTION_R_AXIS;
			break;
		case EditorControl::AXIS_S: // hex plane, southwest to northeast
			active_grid_instance = grid_instance[2];
			edit_plane.normal = Vector3(SQRT3_2, 0, 0.5).normalized();
			cell_depth = 1.5 * cell_size.x;
			grid_transform.rotate(Vector3(0, 1, 0), Math_PI / 3.0);
			grid_transform.translate_local(
					Vector3(is_even_floor * SQRT3_2 * cell_size.x, 0, 0));
			menu_axis = MENU_OPTION_S_AXIS;
			break;
		default:
			ERR_PRINT_ED("unsupported edit plane axis");
			return;
	}
	ERR_FAIL_COND_MSG(
			!active_grid_instance.is_valid(), "no active grid mesh instance");

	// update the depth of the edit plane so it matches the floor, and update
	// the grid transform for the depth.
	edit_plane.d = edit_depth * cell_depth;
	grid_transform.origin += edit_plane.normal * edit_plane.d;

	// shift the edit plane a little into the cell to prevent floating point
	// errors from causing the raycast to fall into the lower cell.  Note we
	// only need to do this when the grid is drawn along the edge of a cell,
	// so the Y & X axis, or any square shape cell.  Hex cells draw the grid
	// through the middle of the cells for Q/R/S.
	if (edit_axis == EditorControl::AXIS_Y ||
			edit_axis == EditorControl::AXIS_X) {
		edit_plane.d += cell_depth * 0.1;
	}

	// make the editing grid visible
	RenderingServer::get_singleton()->instance_set_visible(
			active_grid_instance, true);
	RenderingServer::get_singleton()->instance_set_transform(
			active_grid_instance,
			hex_map->get_global_transform() * grid_transform);
}

// create a mesh and draw a grid of hexagonal cells on it
void HexMapEditorPlugin::_draw_hex_grid(
		RID p_mesh_id, const Vector3 &p_cell_size) {
	// create the points that make up the top of a hex cell
	Vector<Vector3> shape_points;
	shape_points.append(Vector3(0.0, 0, -1.0) * p_cell_size);
	shape_points.append(Vector3(-SQRT3_2, 0, -0.5) * p_cell_size);
	shape_points.append(Vector3(-SQRT3_2, 0, 0.5) * p_cell_size);
	shape_points.append(Vector3(0.0, 0, 1.0) * p_cell_size);
	shape_points.append(Vector3(SQRT3_2, 0, 0.5) * p_cell_size);
	shape_points.append(Vector3(SQRT3_2, 0, -0.5) * p_cell_size);

	PackedVector3Array grid_points;
	TypedArray<Vector3i> cells = hex_map->local_region_to_map(
			Vector3i(-GRID_CURSOR_SIZE * Math_SQRT3 * p_cell_size.x, 0,
					-GRID_CURSOR_SIZE * 1.625 * p_cell_size.x),
			Vector3i(GRID_CURSOR_SIZE * Math_SQRT3 * p_cell_size.x, 0,
					GRID_CURSOR_SIZE * 1.625 * p_cell_size.x));
	for (int i = 0; i < cells.size(); i++) {
		Vector3 center = hex_map->map_to_local(cells[i]);

		for (int j = 1; j < shape_points.size(); j++) {
			grid_points.append(center + shape_points[j - 1]);
			grid_points.append(center + shape_points[j]);
		}
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;
	RenderingServer::get_singleton()->mesh_add_surface_from_arrays(
			p_mesh_id, RenderingServer::PRIMITIVE_LINES, d);
	RenderingServer::get_singleton()->mesh_surface_set_material(
			p_mesh_id, 0, grid_mat->get_rid());
}

// create a mesh and draw a grid for R-Axis cells
void HexMapEditorPlugin::_draw_hex_r_axis_grid(
		RID p_mesh_id, const Vector3 &p_cell_size) {
	PackedVector3Array grid_points;

	// draw horizontal lines
	for (int y_index = -GRID_CURSOR_SIZE; y_index <= GRID_CURSOR_SIZE;
			y_index++) {
		real_t y = y_index * p_cell_size.y;
		grid_points.append(
				Vector3(0, y, -GRID_CURSOR_SIZE * 1.625 * p_cell_size.x));
		grid_points.append(
				Vector3(0, y, GRID_CURSOR_SIZE * 1.625 * p_cell_size.x));
	}

	// for vertical lines, we'll need to know where the center of the cell is
	// for a line along the Z axis.
	TypedArray<Vector3i> cells = hex_map->local_region_to_map(
			Vector3(0, 0.001, -GRID_CURSOR_SIZE * 1.625 * p_cell_size.x),
			Vector3(0, 0.002, GRID_CURSOR_SIZE * 1.625 * p_cell_size.x));

	// use the cell list to draw the vertical lines
	for (int i = 0; i < cells.size(); i++) {
		Vector3i cell = cells[i];

		// grab the z coordinate for the center of the cell
		real_t z = hex_map->map_to_local(cell).z;

		// Adjust from the center of the cell to where the line should fall.
		// We're drawing lines at 1 radius, then 2 radius apart, alternating.
		if ((cell.z & 1) == 0) {
			z += p_cell_size.x;
		} else {
			z += p_cell_size.x / 2;
		}

		grid_points.append(Vector3(0, -GRID_CURSOR_SIZE * p_cell_size.y, z));
		grid_points.append(Vector3(0, GRID_CURSOR_SIZE * p_cell_size.y, z));
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;
	RenderingServer::get_singleton()->mesh_add_surface_from_arrays(
			p_mesh_id, RenderingServer::PRIMITIVE_LINES, d);
	RenderingServer::get_singleton()->mesh_surface_set_material(
			p_mesh_id, 0, grid_mat->get_rid());
}

void HexMapEditorPlugin::_draw_plane_grid(RID p_mesh_id,
		const Vector3 &p_axis_n1, const Vector3 &p_axis_n2,
		const Vector3 &cell_size) {
	PackedVector3Array grid_points;

	Vector3 axis_n1 = p_axis_n1 * cell_size;
	Vector3 axis_n2 = p_axis_n2 * cell_size;

	for (int j = -GRID_CURSOR_SIZE; j <= GRID_CURSOR_SIZE; j++) {
		for (int k = -GRID_CURSOR_SIZE; k <= GRID_CURSOR_SIZE; k++) {
			Vector3 p = axis_n1 * j + axis_n2 * k;

			Vector3 pj = axis_n1 * (j + 1) + axis_n2 * k;

			Vector3 pk = axis_n1 * j + axis_n2 * (k + 1);

			grid_points.push_back(p);
			grid_points.push_back(pk);

			grid_points.push_back(p);
			grid_points.push_back(pj);
		}
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RS::ARRAY_VERTEX] = grid_points;
	RenderingServer::get_singleton()->mesh_add_surface_from_arrays(
			p_mesh_id, RenderingServer::PRIMITIVE_LINES, d);
	RenderingServer::get_singleton()->mesh_surface_set_material(
			p_mesh_id, 0, grid_mat->get_rid());
}

void HexMapEditorPlugin::_draw_grids(const Vector3 &p_cell_size) {
	for (int i = 0; i < 3; i++) {
		RS::get_singleton()->mesh_clear(grid_mesh[i]);
	}

	real_t radius = p_cell_size.x;
	Vector3 cell_size =
			Vector3(Math_SQRT3 * radius, p_cell_size.y, Math_SQRT3 * radius);
	_draw_hex_r_axis_grid(grid_mesh[0], p_cell_size);
	_draw_hex_grid(grid_mesh[1], p_cell_size);
	_draw_plane_grid(
			grid_mesh[2], Vector3(1, 0, 0), Vector3(0, 1, 0), cell_size);
}

void HexMapEditorPlugin::_build_selection_meshes() {
	if (selection_tile_mesh.is_valid()) {
		RS::get_singleton()->free_rid(selection_tile_mesh);
		selection_tile_mesh = RID();
	}
	if (selection_multimesh.is_valid()) {
		RS::get_singleton()->free_rid(selection_multimesh);
		selection_multimesh = RID();
	}

	// we can get called when node is null
	if (hex_map == nullptr) {
		return;
	}

	Array mesh_array;
	mesh_array.resize(RS::ARRAY_MAX);
	Array lines_array;
	lines_array.resize(RS::ARRAY_MAX);

	// Ref<CylinderMesh> m;
	// m.instantiate();
	// m->set_top_radius(1.0);
	// m->set_bottom_radius(1.0);
	// m->set_height(1.0);
	// m->set_rings(1);
	// m->set_radial_segments(6);
	// mesh_array[RS::ARRAY_VERTEX] = m->_create_mesh_array();
	//
	// XXX not exposed; construct by hand
	//
	// CylinderMesh::create_mesh_array(mesh_array, 1.0, 1.0, 1, 6, 1);

	/*
	 *               (0)             Y
	 *              /   \            |
	 *           (1)     (5)         o---X
	 *            |       |           \
	 *           (2)     (4)           Z
	 *            | \   / |
	 *            |  (3)  |
	 *            |   |   |
	 *            |  (6)  |
	 *            | / | \ |
	 *           (7)  |  (b)
	 *            |   |   |
	 *           (8)  |  (a)
	 *              \ | /
	 *               (9)
	 */
	mesh_array[RS::ARRAY_VERTEX] = lines_array[RS::ARRAY_VERTEX] =
			PackedVector3Array({
					Vector3(0.0, 0.5, -1.0), // 0
					Vector3(-SQRT3_2, 0.5, -0.5), // 1
					Vector3(-SQRT3_2, 0.5, 0.5), // 2
					Vector3(0.0, 0.5, 1.0), // 3
					Vector3(SQRT3_2, 0.5, 0.5), // 4
					Vector3(SQRT3_2, 0.5, -0.5), // 5
					Vector3(0.0, -0.5, -1.0), // 6
					Vector3(-SQRT3_2, -0.5, -0.5), // 7
					Vector3(-SQRT3_2, -0.5, 0.5), // 8
					Vector3(0.0, -0.5, 1.0), // 9
					Vector3(SQRT3_2, -0.5, 0.5), // 10 (0xa)
					Vector3(SQRT3_2, -0.5, -0.5), // 11 (0xb)
			});

	// clang-format off
	mesh_array[RS::ARRAY_INDEX] = PackedInt32Array({
		// top
		0, 5, 1,
		1, 5, 2,
		2, 5, 4,
		2, 4, 3,
		// bottom
		6, 7, 11,
		11, 7, 8,
		8, 10, 11,
		10, 8, 9,
		// east
		4,  5, 11,
		11, 10, 4,
		// northeast
		5, 0,  6,
		6, 11, 5,
		// northwest
		0, 1, 7,
		7, 6, 0,
		// west
		1, 2, 8,
		8, 7, 1,
		// southwest
		2, 3, 9,
		9, 8, 2,
		// southeast
		3, 4, 10,
		10, 9, 3,
	});
	// clang-format on

	lines_array[RS::ARRAY_INDEX] = PackedInt32Array({
			0, 1, 2, 3, 4, 5, // top
			11, 6, 7, 8, 9, 10, // bottom
			11, 5, 0, 6, // northeast face
			7, 1, 2, 8, // west face
			9, 3, 4, 10, // southeast face
	});

	RenderingServer *rs = RS::get_singleton();
	selection_tile_mesh = rs->mesh_create();
	rs->mesh_add_surface_from_arrays(
			selection_tile_mesh, RS::PRIMITIVE_TRIANGLES, mesh_array);
	rs->mesh_surface_set_material(
			selection_tile_mesh, 0, selection_mesh_mat->get_rid());

	// add lines around the cell
	rs->mesh_add_surface_from_arrays(
			selection_tile_mesh, RS::PRIMITIVE_LINE_STRIP, lines_array);
	rs->mesh_surface_set_material(
			selection_tile_mesh, 1, selection_line_mat->get_rid());

	// create the multimesh for rendering the tile mesh in multiple locations.
	selection_multimesh = rs->multimesh_create();
	rs->multimesh_set_mesh(selection_multimesh, selection_tile_mesh);
}

void HexMapEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			// fired when the plugin is loaded into the editor
			//
			// does not hot-reload
			UtilityFunctions::print("enter tree");
			for (int i = 0; i < 3; i++) {
				grid_mesh[i] = RS::get_singleton()->mesh_create();
				grid_instance[i] =
						RS::get_singleton()->instance_create2(grid_mesh[i],
								get_tree()
										->get_root()
										->get_world_3d()
										->get_scenario());
				RenderingServer::get_singleton()
						->instance_set_layer_mask(grid_instance[i], 1 << 24 /* XXX Node3DEditorViewport::MISC_TOOL_LAYER */);
				RenderingServer::get_singleton()->instance_set_visible(
						grid_instance[i], false);
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			// fired when exiting tree
			UtilityFunctions::print("exit tree xxx");
			for (int i = 0; i < 3; i++) {
				RS::get_singleton()->free_rid(grid_instance[i]);
				RS::get_singleton()->free_rid(grid_mesh[i]);
				grid_instance[i] = RID();
				grid_mesh[i] = RID();
			}

			deselect_tiles();

			if (grid_manager != nullptr) {
				delete grid_manager;
				grid_manager = nullptr;
			}
		} break;

		case NOTIFICATION_PROCESS: {
			ERR_FAIL_COND_MSG(hex_map == nullptr,
					"HexMapEditorPlugin process without HexMap");

			// if the transform of the HexMap node has been changed, update
			// the grid.
			Transform3D transform = hex_map->get_global_transform();
			if (transform != hex_map_global_transform) {
				hex_map_global_transform = transform;
				update_grid();
				_update_selection();
			}
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			// If the user switches applications while the mouse button is
			// pressed, the state will remain pressed when they return to the
			// editor.  This will result in drawing tiles when the mouse button
			// isn't pressed.
			//
			// Workaround is to fire a release event when editor loses focus.
			if (input_action == INPUT_PAINT) {
				Ref<InputEventMouseButton> release;
				release.instantiate();
				release->set_button_index(MouseButton::MOUSE_BUTTON_LEFT);
				_forward_3d_gui_input(nullptr, release);
			}
		} break;
	}
}

void HexMapEditorPlugin::tile_changed(int p_mesh_id) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->clear_tiles();
	if (p_mesh_id != -1) {
		editor_cursor->set_tile(Vector3i(), p_mesh_id);
		editor_cursor->redraw();
	}
}

void HexMapEditorPlugin::plane_changed(int p_plane) {
	edit_depth = p_plane;
	update_grid();
}

void HexMapEditorPlugin::axis_changed(int p_axis) {
	edit_axis = (EditorControl::EditAxis)p_axis;
	grid_manager->set_axis(edit_axis);
	update_grid();
}

void HexMapEditorPlugin::cursor_changed(Variant p_orientation) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->set_orientation(p_orientation);
}

void HexMapEditorPlugin::_bind_methods() {
	ClassDB::bind_method(
			D_METHOD("_configure"), &HexMapEditorPlugin::_configure);
	ClassDB::bind_method(D_METHOD("_set_selection", "active", "begin", "end"),
			&HexMapEditorPlugin::_set_selection);
}

HexMapEditorPlugin::HexMapEditorPlugin() {
	int mw = ProjectSettings::get_singleton()->get_setting(
			"editors/grid_map/palette_min_width", 230);
	Control *ec = memnew(Control);
	ec->set_custom_minimum_size(Size2(mw, 0) * EDSCALE);
	add_child(ec);

	edit_axis = EditorControl::AXIS_Y;
	edit_plane = Plane();
	edit_depth = 0;

	selection_mesh_mat.instantiate();
	selection_mesh_mat->set_albedo(Color(0.7, 0.7, 1.0, 0.2));
	selection_mesh_mat->set_shading_mode(
			StandardMaterial3D::SHADING_MODE_UNSHADED);
	selection_mesh_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	selection_mesh_mat->set_transparency(
			StandardMaterial3D::TRANSPARENCY_ALPHA);

	selection_line_mat.instantiate();
	selection_line_mat->set_albedo(Color(0.7, 0.7, 1.0, 0.8));
	// outer_mat->set_on_top_of_alpha(); ->
	selection_line_mat->set_transparency(
			godot::BaseMaterial3D::TRANSPARENCY_DISABLED);
	selection_line_mat->set_render_priority(
			RenderingServer::MATERIAL_RENDER_PRIORITY_MAX);
	selection_line_mat->set_flag(
			BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);

	selection_line_mat->set_shading_mode(
			StandardMaterial3D::SHADING_MODE_UNSHADED);
	selection_line_mat->set_transparency(
			StandardMaterial3D::TRANSPARENCY_ALPHA);
	selection_line_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

	grid_mat.instantiate();
	grid_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	grid_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	grid_mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	grid_mat->set_flag(
			StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	grid_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	grid_mat->set_albedo(Color(0.8, 0.5, 0.1));

	// palette
	mesh_palette = memnew(MeshLibraryPalette);
	mesh_palette->hide();
	mesh_palette->connect("mesh_changed",
			callable_mp(this, &HexMapEditorPlugin::tile_changed));
	add_control_to_container(
			CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, mesh_palette);

	// context menu in spatial editor
	editor_control = memnew(EditorControl);
	editor_control->hide();
	editor_control->connect("plane_changed",
			callable_mp(this, &HexMapEditorPlugin::plane_changed));
	editor_control->connect("axis_changed",
			callable_mp(this, &HexMapEditorPlugin::axis_changed));
	editor_control->connect("cursor_orientation_changed",
			callable_mp(this, &HexMapEditorPlugin::cursor_changed));
	editor_control->connect("deselect",
			callable_mp(this, &HexMapEditorPlugin::deselect_tiles));
	editor_control->connect("selection_fill",
			callable_mp(this, &HexMapEditorPlugin::selection_fill));
	editor_control->connect("selection_clear",
			callable_mp(this, &HexMapEditorPlugin::selection_clear));
	editor_control->connect("selection_move",
			callable_mp(this, &HexMapEditorPlugin::selection_move));
	editor_control->connect("selection_clone",
			callable_mp(this, &HexMapEditorPlugin::selection_clone));
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, editor_control);
}

HexMapEditorPlugin::~HexMapEditorPlugin() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());

	for (int i = 0; i < 3; i++) {
		if (grid_mesh[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(grid_mesh[i]);
		}
		if (grid_instance[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(grid_instance[i]);
		}
	}
	if (selection_multimesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(selection_multimesh);
	}
	if (selection_tile_mesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(selection_tile_mesh);
	}

	if (editor_cursor != nullptr) {
		delete editor_cursor;
	}
	assert(grid_manager == nullptr);
}
