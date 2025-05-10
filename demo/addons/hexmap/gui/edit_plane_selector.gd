@tool
extends HBoxContainer

# emitted when either the axis or depth changes
signal changed(axis: int, depth: int)

@export var axis: int :
	set(value):
		if not is_node_ready():
			await ready
		%AxisDropdown.selected = value

@export var depth: int :
	set(value):
		if not is_node_ready():
			await ready
		%DepthSpinBox.value = value

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%AxisDropdown.item_selected.connect(func(v): emit_changed())
	%DepthSpinBox.value_changed.connect(func(v): emit_changed())
	%DepthLabelButton.pressed.connect(func(): %DepthSpinBox.value = 0)

func emit_changed() -> void:
	changed.emit(%AxisDropdown.selected, %DepthSpinBox.value)
