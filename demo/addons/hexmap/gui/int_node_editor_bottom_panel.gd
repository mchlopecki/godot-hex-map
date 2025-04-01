@tool

extends VBoxContainer

# emitted when user creates/edits a cell type
signal update_type(value: int, name: String, color: Color)
# emitted when user deletes a cell type
signal delete_type(value: int)
# emitted when user selects a cell type
signal type_changed()
# emitted when an edit axis is set from the dropdown
signal set_axis(axis: int)

# emitted when the edit plane depth is changed
signal edit_plane_changed(axis: int, depth: int)

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

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%TypeSelector.event_bus = self
	%EditPlaneSelector.changed.connect(func(axis: int, depth: int): print("edit plane changed ", axis, ", ", depth); edit_plane_changed.emit(axis, depth))
	# %AxisDropdown.item_selected.connect(func(v: int): set_axis.emit(v); %AxisDropdown.release_focus())
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
