@tool

extends VBoxContainer

# emitted when user creates/edits a cell type
signal update_type(value: int, name: String, color: Color)
# emitted when user deletes a cell type
signal delete_type(value: int)
# emitted when user selects a cell type
signal type_changed()
# emitted when the edit plane axis or depth is changed
signal edit_plane_changed(axis: int, depth: int)
# emitted to rotate cursor (-1 clockwise, 1 counter-clockwise)
signal rotate_cursor(step: int)
# emitted to move selection
signal move_selection
# emitted to clone selection
signal clone_selection
# emitted to clear selected tiles
signal clear_selection
# emitted to fill the selected tiles
signal fill_selection
# emitted when we want to set visibility of tile value visualization
signal set_tiled_map_visibility(visible: bool)

@export var cell_types: Array:
	set(value):
		if not is_node_ready():
			await ready
		%TypeSelector.cell_types = value

@export var selected_tile: int :
	set(value):
		if not is_node_ready():
			await ready
		%TypeSelector.selected_type = value
	get():
		return %TypeSelector.selected_type

@export var edit_plane_axis: int :
	set(value):
		if not is_node_ready():
			await ready
		%EditPlaneSelector.axis = value

@export var edit_plane_depth: int :
	set(value):
		if not is_node_ready():
			await ready
		%EditPlaneSelector.depth = value

@export var cursor_active: bool = false :
	set(value):
		if cursor_active == value:
			return
		%CCWButton.disabled = !value
		%CWButton.disabled = !value
		cursor_active = value
		%FillButton.disabled = !(cursor_active && selection_active)

@export var selection_active: bool = false :
	set(value):
		if selection_active == value:
			return
		%CopyButton.disabled = !value
		%MoveButton.disabled = !value
		%ClearButton.disabled = !value
		selection_active = value
		%FillButton.disabled = !(cursor_active && selection_active)

@export var show_cells: bool = true :
	set(value):
		%ShowCellsButton.button_pressed = value

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%TypeSelector.event_bus = self
	%EditPlaneSelector.changed.connect(func(axis: int, depth: int): edit_plane_changed.emit(axis, depth))
	%CCWButton.pressed.connect(rotate_cursor.emit.bind(1))
	%CWButton.pressed.connect(rotate_cursor.emit.bind(-1))
	%MoveButton.pressed.connect(move_selection.emit)
	%CopyButton.pressed.connect(clone_selection.emit)
	%FillButton.pressed.connect(fill_selection.emit)
	%ClearButton.pressed.connect(clear_selection.emit)
	%ShowCellsButton.toggled.connect(func(v): set_tiled_map_visibility.emit(v))

	# %AxisDropdown.item_selected.connect(func(v: int): set_axis.emit(v); %AxisDropdown.release_focus())
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
