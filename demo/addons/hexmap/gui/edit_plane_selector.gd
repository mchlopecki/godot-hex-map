@tool
extends HBoxContainer

@export var editor_plugin : EditorPlugin :
	set(value):
		if editor_plugin != null:
			editor_plugin.edit_plane_axis_changed \
					.disconnect(_on_edit_plane_axis_changed)
			editor_plugin.edit_plane_depth_changed \
					.disconnect(_on_edit_plane_depth_changed)
		editor_plugin = value
		if editor_plugin == null:
			return

		editor_plugin.edit_plane_axis_changed \
				.connect(_on_edit_plane_axis_changed)
		editor_plugin.edit_plane_depth_changed \
				.connect(_on_edit_plane_depth_changed)

		axis = editor_plugin.edit_plane_axis
		depth = editor_plugin.edit_plane_depth

@export var axis := -1 :
	set(value):
		if value == axis:
			return

		axis = value
		editor_plugin.edit_plane_axis = value

		if not is_node_ready():
			await ready
		%AxisDropdown.selected = value

@export var depth: int :
	set(value):
		if value == depth:
			return

		depth = value
		editor_plugin.edit_plane_depth = value

		if not is_node_ready():
			await ready
		%DepthSpinBox.value = value

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%AxisDropdown.item_selected.connect(func(v): axis = v)
	%DepthSpinBox.value_changed.connect(func(v): depth = v)
	%DepthLabelButton.pressed.connect(func(): depth = 0)

func _on_edit_plane_axis_changed(value: int) -> void:
	axis = value

func _on_edit_plane_depth_changed(value: int) -> void:
	depth = value
