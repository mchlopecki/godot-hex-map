#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/menu_button.hpp"
#include "godot_cpp/classes/spin_box.hpp"
#include <godot_cpp/classes/control.hpp>

using namespace godot;

class EditorControl : public godot::HBoxContainer {
	GDCLASS(EditorControl, godot::HBoxContainer);

	enum Action {
		PLANE_DOWN,
		PLANE_UP,
	};

	enum EditAxis {
		AXIS_Y,
		AXIS_Q, // northwest/southeast
		AXIS_R, // east/west
		AXIS_S, // northeast/southeast
	};

private:
	Label *plane_label = nullptr;
	SpinBox *plane_spin_box = nullptr;
	MenuButton *menu_button = nullptr;

	// plane value for each axis
	int active_axis = AXIS_Y;
	int plane[AXIS_S + 1] = { 0 };

	Ref<Shortcut> editor_shortcut(const String &p_path, const String &p_name,
			Key p_keycode, bool p_physical);
	void set_plane(int p_plane);
	void handle_action(int p_action);

protected:
public:
	static void _bind_methods();

	// handle a keypress event if it matches a shortcut in the menu
	bool handle_keypress(Ref<InputEventKey> p_event);

	EditorControl();
	~EditorControl();
};
