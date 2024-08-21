/**************************************************************************/
/*  grid_map_editor_plugin.h                                              */
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

#pragma once

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/slider.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include "../hex_map.h"
#include "editor_control.h"
#include "mesh_library_palette.h"

using namespace godot;

// Figure out how to add EditorSetting for HexMapEditorPlugin
// Break out MeshLibraryPicker (VBox)
// Break out ContextMenu (HBox); signals back for control
//
// Signals or function calls to EditorPlugins?
//
// XXX BUG while shift-click dragging, if you drag outside of the pane, will
// translate the node we're editing.

// EditorPlugin
//	- has get_editor_interface()
//	- needs EditorInterface for shortcuts & settings
//	- handle viewpane input
//		- mouse movement move cursor
//		- clicks place tiles, do selection
//	- 3d view output
//		- draw grid
//		- draw cursor
//
//
// MeshLibraryPicker (VBox)
//	- Needs Ref<Theme> to get icons
//	- Hbox
//		- LineEdit (filter)
//		- Button (thumbnail)
//		- Button (list)
//  - HSlider (thumbnail size)
//	- ItemList (mesh library palette)
//
// ContextMenu
//	- to get settings/shortcuts need EditorSettings || ProjectSettings
//	- Floor/Layer
//	- Axis setting
//		- maintain per-axis floor here; just signal plane_changed(axis, floor)
//	- Tile/Selection rotation; cursor_changed(orientation?)
//	- Deselect; "select_none"
//	- Fill Selection; "selection_fill"
//	- Copy Selection; "selection_copy"
//	- Cut Selection;  "selection_cut"
//	- enable/disable selection-based menu options
//
// Signals
//  - HexMap
//		- cell_size_changed -> redraw grids, cursor
//		- cell_shape_changed -> redraw grids, cursor
//		- mesh_library_changed -> update palette
//		- map_changed(pos. old, new)
//  - MeshLibrary changed -> update palette
//	- MeshLibraryPicker
//		- Search Box
//			- text_changed -> filter ItemList
//			- gui_input -> up/down/page_up/page_down redirect to ItemList for
//			  selection
//		- Thumbnail Button "pressed" -> set ItemList display mode
//		- List Button "pressed" -> set ItemList display mode
//		- Size Slider "value_changed" -> set ItemList icon size
//		- ItemList "gui_input" -> ctrl + mouse wheel size adjustments
//		- item_selected(index) -> set cursor model
//	- ContextMenu
//		- Floor
//			- value_changed -> to EditorPlugin
//			- mouse_exited -> release floor edit focus
//		- Options Dropdown Menu
//			- id_pressed -> process, then issue "changed"
//		- changed(axis, floor)

class HexMapEditorPlugin : public EditorPlugin {
	GDCLASS(HexMapEditorPlugin, EditorPlugin);

	enum {
		GRID_CURSOR_SIZE = 50
	};

	enum InputAction {
		INPUT_NONE,
		INPUT_PAINT,
		INPUT_ERASE,
		INPUT_PICK,
		INPUT_SELECT,
		INPUT_PASTE,
	};

	enum DisplayMode {
		DISPLAY_THUMBNAIL,
		DISPLAY_LIST
	};

	MeshLibraryPalette *palette = nullptr;
	EditorControl *editor_control = nullptr;
	InputAction input_action = INPUT_NONE;
	double accumulated_floor_delta = 0.0;

	struct SetItem {
		Vector3i position;
		int new_value = 0;
		int new_orientation = 0;
		int old_value = 0;
		int old_orientation = 0;
	};

	List<SetItem> set_items;

	HexMap *node = nullptr;
	// caching the node global transform to detect when the node has been
	// moved/scaled/rotated.
	Transform3D node_global_transform;
	Ref<MeshLibrary> mesh_library = nullptr;

	// plane we're editing cells on; depth comes from edit_floor
	Plane edit_plane;

	// enum EditAxis {
	// 	AXIS_X = 0,
	// 	AXIS_Y,
	// 	AXIS_Z,
	// 	AXIS_Q, // axial hex coordinates northwest/southeast
	// 	AXIS_R, // axial hex coordinates east/west
	// 	AXIS_S, // axial hex coordinates northeast/southwest
	// 	AXIS_MAX,
	// };
	EditorControl::EditAxis edit_axis;
	int edit_depth;

	RID active_grid_instance;
	RID grid_mesh[3];
	RID grid_instance[3];
	RID cursor_instance;

	struct ClipboardItem {
		int cell_item = 0;
		Vector3 position;
		int orientation = 0;
		RID instance;
	};

	List<ClipboardItem> clipboard_items;

	Ref<StandardMaterial3D> indicator_mat;
	Ref<StandardMaterial3D> inner_mat;
	Ref<StandardMaterial3D> outer_mat;

	struct Selection {
		Vector3 begin;
		Vector3 end;
		bool active = false;
		Vector<Vector3i> cells;
	} selection;
	Selection last_selection;
	RID selection_tile_mesh;
	RID selection_multimesh;
	RID selection_multimesh_instance;

	// orientation of the paste indicator; uses orientation from GridMap
	int paste_orientation = 0;

	bool cursor_visible = false;
	Transform3D cursor_transform;

	// cell index for the pointer
	Vector3i pointer_cell;

	int display_mode = DISPLAY_THUMBNAIL;
	int selected_palette = -1;
	int cursor_rot = 0;

	enum Menu {
		MENU_OPTION_NEXT_LEVEL,
		MENU_OPTION_PREV_LEVEL,
		MENU_OPTION_LOCK_VIEW,
		MENU_OPTION_X_AXIS,
		MENU_OPTION_Y_AXIS,
		MENU_OPTION_Z_AXIS,
		MENU_OPTION_Q_AXIS,
		MENU_OPTION_R_AXIS,
		MENU_OPTION_S_AXIS,
		MENU_OPTION_ROTATE_AXIS_CW,
		MENU_OPTION_ROTATE_AXIS_CCW,
		MENU_OPTION_CURSOR_ROTATE_Y,
		MENU_OPTION_CURSOR_ROTATE_X,
		MENU_OPTION_CURSOR_ROTATE_Z,
		MENU_OPTION_CURSOR_BACK_ROTATE_Y,
		MENU_OPTION_CURSOR_BACK_ROTATE_X,
		MENU_OPTION_CURSOR_BACK_ROTATE_Z,
		MENU_OPTION_CURSOR_CLEAR_ROTATION,
		MENU_OPTION_PASTE_SELECTS,
		MENU_OPTION_SELECTION_DUPLICATE,
		MENU_OPTION_SELECTION_CUT,
		MENU_OPTION_SELECTION_CLEAR,
		MENU_OPTION_SELECTION_FILL,
		MENU_OPTION_GRIDMAP_SETTINGS

	};

	struct AreaDisplay {
		RID mesh;
		RID instance;
	};

	void update_grid(); // Change which and where the grid is displayed
	void _draw_hex_grid(RID p_grid, const Vector3 &p_cell_size);
	void _draw_hex_r_axis_grid(RID p_grid, const Vector3 &p_cell_size);
	void _draw_plane_grid(RID p_grid, const Vector3 &p_axis_n1, const Vector3 &p_axis_n2, const Vector3 &p_cell_size);
	void _draw_grids(const Vector3 &p_cell_size);
	void _update_cell_shape(const HexMap::CellShape cell_shape);
	void _update_options_menu();
	void _build_selection_meshes();
	void _configure();
	void _menu_option(int);
	void _update_mesh_library();
	void _item_selected_cbk(int idx);
	void _update_cursor_transform();
	void _update_cursor_instance();
	void _update_theme();

	// callbacks used by signals from EditorControl
	void mesh_changed(int p_mesh_id);
	void plane_changed(int p_axis);
	void axis_changed(int p_axis);
	void cursor_changed(int p_rotation, bool p_flip);
	void deselect_tiles();
	void selection_fill();
	void selection_clear();
	void selection_move();
	void selection_clone();

	void _icon_size_changed(float p_value);

	void _clear_clipboard_data();
	void _set_clipboard_data();
	void _update_paste_indicator();
	void _do_paste();
	void _update_selection();
	void _set_selection(bool p_active, const Vector3 &p_begin = Vector3(), const Vector3 &p_end = Vector3());

	void _floor_mouse_exited();

	void _delete_selection();

	bool do_input_action(Camera3D *p_camera, const Point2 &p_point, bool p_click);

	friend class GridMapEditorPlugin;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual void _make_visible(bool visible) override;
	virtual bool _handles(Object *object) const override;
	virtual void _edit(Object *p_object) override;
	virtual int32_t _forward_3d_gui_input(Camera3D *viewport_camera, const Ref<InputEvent> &event) override;

	HexMapEditorPlugin();
	~HexMapEditorPlugin();
};

// class GridMapEditorPlugin : public EditorPlugin {
// 	GDCLASS(GridMapEditorPlugin, EditorPlugin);
//
// 	GridMapEditor *grid_map_editor = nullptr;
//
// protected:
// 	void _notification(int p_what);
//
// public:
// 	// XXX no such virtual function to override
// 	// virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override { return grid_map_editor->forward_spatial_input_event(p_camera, p_event); }
// 	// virtual String get_name() const override { return "GridMap"; }
// 	// bool has_main_screen() const override { return false; }
// 	// virtual void edit(Object *p_object) override;
// 	// virtual bool handles(Object *p_object) const override;
// 	// virtual void make_visible(bool p_visible) override;
//
// 	GridMapEditorPlugin();
// 	~GridMapEditorPlugin();
// };
