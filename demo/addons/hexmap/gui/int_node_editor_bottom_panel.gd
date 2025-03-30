@tool

extends VBoxContainer

# emitted when user creates/edits a cell type
signal update_type(value: int, name: String, color: Color)
# emitted when user deletes a cell type
signal delete_type(value: int)
# emitted when user selects a cell type
signal type_changed()
signal set_axis(axis: int)

@export var cell_types: Array:
	set(value):
		%TypeSelector.cell_types = value

@export var selected_type: int :
	set(value):
		%TypeSelector.selected_type = value
	get():
		return %TypeSelector.selected_type

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%TypeSelector.event_bus = self
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
