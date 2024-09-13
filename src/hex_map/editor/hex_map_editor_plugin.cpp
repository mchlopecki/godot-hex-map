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
#include "editor_control.h"
#include "editor_cursor.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input_event_mouse.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "selection_manager.h"

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


void HexMapEditorPlugin::deselect_cells() {
	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action("HexMap: deselect cells");

	undo_redo->add_do_method(this, "deselect_cells");

	// clear and select cells
	undo_redo->add_undo_method(this, "deselect_cells");
	for (const HexMapCellId &cell : selection_manager->get_cells()) {
		undo_redo->add_undo_method(this, "select_cell", (Vector3i)cell);
	}

	undo_redo->commit_action();
}

void HexMapEditorPlugin::_deselect_cells() {
	selection_manager->clear();
	editor_control->update_selection_menu(false);
}

void HexMapEditorPlugin::_select_cell(Vector3i cell) {
	selection_manager->set_cell(cell);
	// XXX slow, only need to do this once
	editor_control->update_selection_menu(true);
}

bool HexMapEditorPlugin::do_input_action(
		Camera3D *p_camera, const Point2 &p_point, bool p_click) {
	// if (!editor_cursor) {
	// 	return false;
	// }
	// Vector3 edit_plane_point;
	// if (!editor_cursor->update(p_camera, p_point, &edit_plane_point)) {
	// 	return false;
	// }
	// pointer_cell = editor_cursor->get_cell();
	//
	// if (input_action == INPUT_PASTE) {
	// 	// _update_paste_indicator();
	//
	// } else if (input_action == INPUT_SELECT) {
	// 	if (p_click) {
	// 		selection.begin = edit_plane_point;
	// 	}
	// 	selection.end = edit_plane_point;
	// 	selection.active = true;
	// 	_update_selection();
	// 	editor_control->update_selection_menu(true);
	//
	// 	return true;
	// } else if (input_action == INPUT_PICK) {
	// 	// XXX allow picking from tiles on map,
	// 	int item = hex_map->get_cell_item(pointer_cell);
	// 	if (item >= 0) {
	// 		// XXX should send a signal back when called; so we probably don't
	// 		// need to set it ourselves here.
	// 		mesh_palette->set_mesh(item);
	// 	}
	// 	return true;
	// }
	//
	// if (input_action == INPUT_PAINT) {
	// 	// XXX maybe a custom method for this to get paint tile
	// 	List<EditorCursor::CursorCell> tiles = editor_cursor->get_tiles();
	// 	if (tiles.is_empty()) {
	// 		return true;
	// 	}
	// 	EditorCursor::CursorCell &tile = tiles[0];
	// 	// XXX disallow painting outside of selection box if selected
	// 	SetItem si;
	// 	si.position = tile.cell_id_live;
	// 	si.new_value = tile.tile;
	// 	si.new_orientation = tile.orientation;
	// 	si.old_value = hex_map->get_cell_item(pointer_cell);
	// 	si.old_orientation = hex_map->get_cell_item_orientation(pointer_cell);
	// 	set_items.push_back(si);
	// 	hex_map->set_cell_item(pointer_cell, tile.tile, tile.orientation);
	// 	return true;
	// } else if (input_action == INPUT_ERASE) {
	// 	SetItem si;
	// 	si.position =
	// 			Vector3i(pointer_cell[0], pointer_cell[1], pointer_cell[2]);
	// 	si.new_value = -1;
	// 	si.new_orientation = 0;
	// 	si.old_value = hex_map->get_cell_item(pointer_cell);
	// 	si.old_orientation = hex_map->get_cell_item_orientation(pointer_cell);
	// 	set_items.push_back(si);
	// 	hex_map->set_cell_item(pointer_cell, -1);
	// 	return true;
	// }
	//
	return false;
}

void HexMapEditorPlugin::selection_clear() {
	ERR_FAIL_COND_MSG(selection_manager->is_empty(), "no cells selected");

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("GridMap Delete Selection"));

	for (const Vector3i cell : selection_manager->get_cells()) {
		undo_redo->add_do_method(
				hex_map, "set_cell_item", cell, HexMap::INVALID_CELL_ITEM);
		undo_redo->add_undo_method(hex_map, "set_cell_item", cell,
				hex_map->get_cell_item(cell),
				hex_map->get_cell_item_orientation(cell));
	}

	undo_redo->commit_action();
}

void HexMapEditorPlugin::selection_fill() {
	ERR_FAIL_COND_MSG(selection_manager->is_empty(), "no cells selected");

	List<EditorCursor::CursorCell> tiles = editor_cursor->get_tiles();
	ERR_FAIL_COND_MSG(
			tiles.size() > 1, "cursor has more than one tile; need one");
	ERR_FAIL_COND_MSG(tiles.size() == 0, "cursor has no tiles; need one");
	EditorCursor::CursorCell &tile = tiles[0];

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("GridMap Fill Selection"));

	for (const Vector3i cell : selection_manager->get_cells()) {
		undo_redo->add_do_method(
				hex_map, "set_cell_item", cell, tile.tile, tile.orientation);
		undo_redo->add_undo_method(hex_map, "set_cell_item", cell,
				hex_map->get_cell_item(cell),
				hex_map->get_cell_item_orientation(cell));
	}

	undo_redo->commit_action();
}

void HexMapEditorPlugin::selection_begin_move() {
	ERR_FAIL_COND(editor_cursor == nullptr);

	// make the current pointer cell the origin cell in the cursor.

	editor_cursor->clear_tiles();
	editor_control->reset_orientation();

	Vector<HexMapCellId> cells = selection_manager->get_cells();
	if (cells.is_empty()) {
		return;
	}

	for (const Vector3i &cell : cells) {
		int cell_item = hex_map->get_cell_item(cell);
		if (cell_item == HexMap::INVALID_CELL_ITEM) {
			continue;
		}
		editor_cursor->set_tile(cell - pointer_cell, cell_item,
				hex_map->get_cell_item_orientation(cell));
	}
	editor_cursor->update(true);

	input_action = INPUT_PASTE;
}

void HexMapEditorPlugin::selection_clone() {
	// XXX need to distinguish clone from move
	// when move completes, save the clipboard to allow quick duplicate
	selection_begin_move();
	_deselect_cells();
}

void HexMapEditorPlugin::selection_move() {
	// XXX need to distinguish move from clone
	// when escape is pressed, need to undo clear selection/ret clipboard
	// contents.
	selection_begin_move();
	// XXX need to be able to undo this
	selection_clear();
	_deselect_cells();
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
	editor_cursor->update(true);
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

	// try to handle any key event as a shortcut first
	// XXX add filters to EditorControl callbacks to ensure we only perform
	// certian actions during the appropriate input state.  Example, do not
	// move or clone during painting.
	Ref<InputEventKey> key_event = p_event;
	if (key_event.is_valid() && editor_control->handle_keypress(key_event)) {
		return AFTER_GUI_INPUT_STOP;
	}
	bool escape_pressed = key_event.is_valid() &&
		key_event->get_keycode() == Key::KEY_ESCAPE &&
		key_event->is_pressed();

	// if it's any type of mouse event, update the editor cursor
	Ref<InputEventMouse> mouse_event = p_event;
	if (mouse_event.is_valid()) {
		editor_cursor->update(p_camera, mouse_event->get_position(), nullptr);
	}

	// grab the common mouse button up/downs to simplify the switch statement
	// below
	Ref<InputEventMouseButton> mouse_button_event = p_event;
	bool mouse_left_pressed = mouse_button_event.is_valid() &&
			mouse_button_event->get_button_index() ==
					MouseButton::MOUSE_BUTTON_LEFT &&
			mouse_button_event->is_pressed();
	bool mouse_left_released = mouse_button_event.is_valid() &&
			mouse_button_event->get_button_index() ==
					MouseButton::MOUSE_BUTTON_LEFT &&
			mouse_button_event->is_released();
	bool mouse_right_pressed = mouse_button_event.is_valid() &&
			mouse_button_event->get_button_index() ==
					MouseButton::MOUSE_BUTTON_RIGHT &&
			mouse_button_event->is_pressed();
	bool shift_pressed = mouse_button_event.is_valid() &&
			mouse_button_event->is_shift_pressed();


	switch (input_state) {
		// transitions:
		// - SELECTING (shift + left button down)
		// - PAINTING (left button down)
		// - ERASING (right button down)
		// actions:
		// - clear selection (escape && have selection)
		// - clear cursor (escape)
		case INPUT_STATE_DEFAULT: 
			if (mouse_left_pressed && shift_pressed) {
				editor_cursor->hide();
				input_state = INPUT_STATE_SELECTING;
				last_selection = selection_manager->get_cells();
				selection_anchor = editor_cursor->get_pos();
				selection_manager->clear();
				selection_manager->set_cell(editor_cursor->get_cell());
				return AFTER_GUI_INPUT_STOP;
			} else if (mouse_left_pressed) {
				input_state = INPUT_STATE_PAINTING;
				return AFTER_GUI_INPUT_STOP;
			} else if (mouse_right_pressed) {
				input_state = INPUT_STATE_ERASING;
				return AFTER_GUI_INPUT_STOP;
			} else if (escape_pressed) {
				if (!selection_manager->is_empty()) {
					deselect_cells();
				} else {
					mesh_palette->clear_selection();
				}
				return AFTER_GUI_INPUT_STOP;
			} 
			break;
		case INPUT_STATE_PAINTING:
			input_state = INPUT_STATE_DEFAULT;
			break;
		case INPUT_STATE_ERASING:
			input_state = INPUT_STATE_DEFAULT;
			break;
		// transitions:
		// - DEFAULT (left button up)
		// - DEFAULT (escape); clear selection
		// actions:
		// - update selection (mouse event)
		case INPUT_STATE_SELECTING:
			if (mouse_event.is_valid()) {
				auto cells = hex_map->local_region_to_map(selection_anchor,
											  editor_cursor->get_pos());
				selection_manager->clear();
				selection_manager->set_cells(cells);
			}
			if (mouse_left_released) {
				EditorUndoRedoManager *undo_redo = get_undo_redo();
				undo_redo->create_action("HexMap: select cells");
				// do will set the selection
				undo_redo->add_do_method(this, "deselect_cells");
				for (const HexMapCellId &cell : selection_manager->get_cells()) {
					undo_redo->add_do_method(this, "select_cell", (Vector3i)cell);
				}
				// undo will restore last selection
				undo_redo->add_undo_method(this, "deselect_cells");
				for (const HexMapCellId &cell : last_selection) {
					undo_redo->add_undo_method(this, "select_cell", (Vector3i)cell);
				}
				undo_redo->commit_action();

				editor_cursor->show();
				input_state = INPUT_STATE_DEFAULT;
				return AFTER_GUI_INPUT_STOP;
			} else if (escape_pressed) {
				selection_manager->clear();
				selection_manager->set_cells(last_selection);
				editor_cursor->show();
				input_state = INPUT_STATE_DEFAULT;
				return AFTER_GUI_INPUT_STOP;
			}
			break;
		case INPUT_STATE_MOVING:
			input_state = INPUT_STATE_DEFAULT;
			break;
		case INPUT_STATE_CLONING:
			input_state = INPUT_STATE_DEFAULT;
			break;
	}

	return EditorPlugin::AFTER_GUI_INPUT_PASS;

	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		// XXX allow floor change with mouse-based control
		// XXX DOES NOT WRH
		if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_UP &&
				mb->is_shift_pressed() && mb->is_pressed()) {
			editor_control->set_plane(editor_control->get_plane() + 1);
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		} else if (mb->get_button_index() ==
						MouseButton::MOUSE_BUTTON_WHEEL_DOWN &&
				mb->is_shift_pressed() && mb->is_pressed()) {
			editor_control->set_plane(editor_control->get_plane() - 1);
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		}

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
					// last_selection = selection;
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
				// } else if (selection.active) {
				// 	deselect_tiles();
				// 	return EditorPlugin::AFTER_GUI_INPUT_STOP;
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

			// if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT &&
			// 		input_action == INPUT_SELECT) {
			// 	EditorUndoRedoManager *undo_redo = get_undo_redo();
			// 	undo_redo->create_action(TTR("GridMap Selection"));
			// 	undo_redo->add_do_method(this, "_set_selection",
			// 			selection.active, selection.begin, selection.end);
			// 	undo_redo->add_undo_method(this, "_set_selection",
			// 			last_selection.active, last_selection.begin,
			// 			last_selection.end);
			// 	undo_redo->commit_action();
			// 	editor_cursor->show();
			// }

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

	// XXX need to test this
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
}

void HexMapEditorPlugin::_edit(Object *p_object) {
	if (hex_map) {
		hex_map->disconnect(SNAME("cell_size_changed"),
				callable_mp(this, &HexMapEditorPlugin::cell_size_changed));
		hex_map->disconnect("changed",
				callable_mp(this, &HexMapEditorPlugin::_update_mesh_library));
		if (mesh_library.is_valid()) {
			mesh_library = Ref<MeshLibrary>();
		}

		// we use delete here because EditorCursor is not a Godot Object
		// subclass, so its destructor is not called with memfree().
		delete editor_cursor;
		editor_cursor = nullptr;
		delete selection_manager;
		selection_manager = nullptr;
	}

	// XXX works when no connect() calls made in constructor; waiting on 4.3
	// which will have https://github.com/godotengine/godot-cpp/pull/1446
	hex_map = Object::cast_to<HexMap>(p_object);

	input_action = INPUT_NONE;
	mesh_palette->clear_selection();

	// spatial_editor =
	// Object::cast_to<Node3DEditorPlugin>(EditorNode::get_singleton()->get_editor_plugin_screen());

	if (!hex_map) {
		set_process(false);
		return;
	}

	// not a godot Object subclass, so `new` instead of `memnew()`
	editor_cursor = new EditorCursor(hex_map);
	selection_manager = new SelectionManager(hex_map);

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

	// hex_map->connect(SNAME("cell_size_changed"),
	// 		callable_mp(this, &HexMapEditorPlugin::_draw_grids));
	hex_map->connect(SNAME("cell_size_changed"),
			callable_mp(this, &HexMapEditorPlugin::cell_size_changed));
	hex_map->connect("changed",
			callable_mp(this, &HexMapEditorPlugin::_update_mesh_library));
	_update_mesh_library();
}

void HexMapEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			// we cache the global transform of the HexMap.  When the global
			// transform changes, we'll update our visuals accordingly.
			static Transform3D cached_transform;
			ERR_FAIL_COND_MSG(hex_map == nullptr,
					"HexMapEditorPlugin process without HexMap");

			// if the transform of the HexMap node has been changed, update
			// the grid.
			Transform3D transform = hex_map->get_global_transform();
			if (transform != cached_transform) {
				cached_transform = transform;
				editor_cursor->update(true);
				selection_manager->redraw_selection();
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

void HexMapEditorPlugin::cell_size_changed(Vector3 cell_size) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->cell_size_changed();
}

void HexMapEditorPlugin::tile_changed(int p_mesh_id) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->clear_tiles();
	if (p_mesh_id != -1) {
		editor_cursor->set_tile(Vector3i(), p_mesh_id);
		editor_cursor->update(true);
	}
}

void HexMapEditorPlugin::plane_changed(int p_plane) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->set_depth(p_plane);
}

void HexMapEditorPlugin::axis_changed(int p_axis) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->set_axis((EditorControl::EditAxis)p_axis);
}

void HexMapEditorPlugin::cursor_changed(Variant p_orientation) {
	ERR_FAIL_COND_MSG(editor_cursor == nullptr, "editor_cursor not present");
	editor_cursor->set_orientation(p_orientation);
}

void HexMapEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("deselect_cells"),
	 		&HexMapEditorPlugin::_deselect_cells);
	ClassDB::bind_method(D_METHOD("select_cell", "cell"),
	 		&HexMapEditorPlugin::_select_cell);
	// ClassDB::bind_method(D_METHOD("_set_selection", "active", "begin", "end"),
	// 		&HexMapEditorPlugin::_set_selection);
}

HexMapEditorPlugin::HexMapEditorPlugin() {
	int mw = ProjectSettings::get_singleton()->get_setting(
			"editors/grid_map/palette_min_width", 230);
	Control *ec = memnew(Control);
	ec->set_custom_minimum_size(Size2(mw, 0) * EDSCALE);
	add_child(ec);

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
			callable_mp(this, &HexMapEditorPlugin::deselect_cells));
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

	if (editor_cursor != nullptr) {
		delete editor_cursor;
	}
	if (selection_manager != nullptr) {
		delete selection_manager;
	}
	// editor_control & mesh_palette are cleaned up automatically.
}
