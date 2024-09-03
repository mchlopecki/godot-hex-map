#include "../tile_orientation.h"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/menu_button.hpp"
#include "godot_cpp/classes/spin_box.hpp"
#include <godot_cpp/classes/control.hpp>

#pragma once

using namespace godot;

class EditorControl : public godot::HBoxContainer {
	GDCLASS(EditorControl, godot::HBoxContainer);

public:
	enum EditAxis {
		AXIS_X,
		AXIS_Y,
		AXIS_Q, // northwest/southeast
		AXIS_R, // east/west
		AXIS_S, // northeast/southeast
	};

private:
	enum Action {
		ACTION_PLANE_DOWN,
		ACTION_PLANE_UP,
		ACTION_AXIS_X,
		ACTION_AXIS_Y,
		ACTION_AXIS_Q,
		ACTION_AXIS_R,
		ACTION_AXIS_S,
		ACTION_AXIS_ROTATE_CW,
		ACTION_AXIS_ROTATE_CCW,
		ACTION_TILE_ROTATE_CW,
		ACTION_TILE_ROTATE_CCW,
		ACTION_TILE_FLIP,
		ACTION_TILE_RESET,
		ACTION_SELECTION_FILL,
		ACTION_SELECTION_CLEAR,
		ACTION_SELECTION_CLONE,
		ACTION_SELECTION_MOVE,
		ACTION_DESELECT,
	};

private:
	Label *plane_label = nullptr;
	SpinBox *plane_spin_box = nullptr;
	MenuButton *menu_button = nullptr;

	// plane value for each axis
	EditAxis active_axis = AXIS_Y;
	int plane[AXIS_S + 1] = { 0 };
	TileOrientation cursor_orientation;

	Ref<Shortcut> editor_shortcut(const String &p_path, const String &p_name,
			Key p_keycode, bool p_physical);
	void set_plane(int p_plane);
	void handle_action(int p_action);

protected:
public:
	static void _bind_methods();

	EditAxis get_active_axis() { return active_axis; }
	int get_plane() { return plane[active_axis]; }

	// handle a keypress event if it matches a shortcut in the menu
	bool handle_keypress(Ref<InputEventKey> p_event);

	// used to update the dropdown menu when there is an active selection
	void update_selection_menu(bool p_active, bool p_duplicate = false);

	// reset cursor orientation
	void reset_orientation() {
		cursor_orientation = TileOrientation::Upright0;
	}

	EditorControl();
	~EditorControl();
};
