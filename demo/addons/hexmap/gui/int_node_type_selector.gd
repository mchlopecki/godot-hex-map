@tool

extends VBoxContainer

# emitted when a type is selected
signal selected(value: int)
# emitted for both add & update type; value will be -1 for add
signal updated(value: int, name: String, color: Color)
# emitted when a type is deleted
signal deleted(value: int)

@export var cell_types : Array :
	set(value):
		cell_types = value
		rebuild_type_list()

@export var selected_type: int = -1 :
	set(value):
		if value != selected_type:
			for child in %CellTypes.get_children():
				if child.value == value:
					child.selected = true
					selected_color = child.color
				else:
					child.selected = false
			selected_type = value
			selected.emit(value)

@export var selected_color: Color

@export var read_only := false

var context_menu_node: Node

const cell_type_button: PackedScene = preload("int_node_cell_type_button.tscn")
const cell_type_form: PackedScene = preload("int_node_cell_type_form.tscn")

func rebuild_type_list() -> void:
	for child in %CellTypes.get_children():
		child.queue_free()

	cell_types.sort_custom(func(a, b):
		return a.name.naturalnocasecmp_to(b.name) < 0)
	for type in cell_types:
		var cell = cell_type_button.instantiate()
		cell.value = type.value
		cell.type = type.name
		cell.color = type.color
		cell.pressed.connect(func(): selected_type = type.value)
		%CellTypes.add_child(cell)

func open_context_menu(pos: Vector2) -> void:
	var type_node
	for child in %CellTypes.get_children():
		if child.hovered:
			type_node = child
			break
	if !type_node:
		return

	context_menu_node = type_node
	%ContextMenu.position = pos
	%ContextMenu.visible = true

func context_menu_visibility_changed() -> void:
	var visible = %ContextMenu.visible
	for child in %CellTypes.get_children():
		child.frozen = visible

func context_menu_index_pressed(index: int) -> void:
	match index:
		0:
			edit_type(context_menu_node)
		1:
			deleted.emit(context_menu_node.value)
			rebuild_type_list()
		_:
			push_error("Unknown context menu index ", index)

func cell_type_form_submitted(value: int, name: String, color: Color) -> void:
	updated.emit(value, name, color)

func append_new_type_form() -> void:
	var form := cell_type_form.instantiate()
	form.color = Color.from_hsv(
			randf(),
			randf_range(0.2, 0.6),
			randf_range(0.9, 1.0))
	form.submitted.connect(cell_type_form_submitted)
	%CellTypes.add_child(form)
	form.grab_focus()

func edit_type(type_node: Node) -> void:
	var form := cell_type_form.instantiate()
	form.value = type_node.value
	form.type = type_node.type
	form.color = type_node.color
	form.submitted.connect(cell_type_form_submitted)
	form.cancelled.connect(rebuild_type_list)
	type_node.add_sibling(form)
	type_node.queue_free()
	form.grab_focus()
	
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%ContextMenu.visibility_changed.connect(context_menu_visibility_changed)
	%ContextMenu.id_pressed.connect(context_menu_index_pressed)
	if read_only:
		%AddTypeFormButton.visible = false
	else:
		%AddTypeFormButton.visible = true
		%AddTypeFormButton.pressed.connect(append_new_type_form)
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.is_pressed() && event.button_index == MOUSE_BUTTON_RIGHT && !read_only:
			open_context_menu(event.position)
