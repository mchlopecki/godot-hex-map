@tool
extends Control

# emitted when user clicks on one of the cell buttons
signal cell_clicked(id: HexMapCellId, button: MouseButton)

static var cell_button: PackedScene = preload("hex_cell_button.tscn")
const cell_radius := 18
var cell_buttons := {}

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var node : HexMapInt = HexMapInt.new()
	node.set_cell_radius(cell_radius * EditorInterface.get_editor_scale())

	# center point of the control
	var control_center = _get_minimum_size() / 2
	print("size ", _get_minimum_size(), ", center ", control_center);

	var cells = HexMapCellId.new().get_neighbors(2, true)
	for cell_id in cells:
		if cell_id.get_y() != 0:
			continue
		var center3 = node.get_cell_center(cell_id)
		var center = Vector2(center3.x, center3.z) + control_center
		var button = cell_button.instantiate()

		if cell_id.get_q() == 0 && cell_id.get_r() == 0:
			print("Setting border color")
			button.border_color = Color.WHITE

		# XXX get this vector offset from HexCellButton
		button.set_position(center - button._get_minimum_size()/2)
		button.pressed.connect(cell_pressed.bind(cell_id))
		cell_buttons[cell_id.as_int()] = button
		add_child(button)
	pass

func _get_minimum_size() -> Vector2:
	return Vector2(5 * cell_radius * sqrt(3), 8 * cell_radius) * EditorInterface.get_editor_scale()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func cell_pressed(id: HexMapCellId) -> void:
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		cell_clicked.emit(id, MOUSE_BUTTON_LEFT)
	elif Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		cell_clicked.emit(id, MOUSE_BUTTON_RIGHT)
	else:
		push_error("unknown mouse button pressed")

func set_cell(id: HexMapCellId, color, icon) -> void:
	var button = cell_buttons[id.as_int()]
	if !button:
		print("unknown cell_id ", id)
		return
	button.color = color
	match icon:
		null:
			button.icon = HexCellButton.Icon.None
		"not":
			button.icon = HexCellButton.Icon.X
		"some":
			button.icon = HexCellButton.Icon.Star
		_:
			push_error("unsupported cell icon ", icon)

func reset() -> void:
	for button in cell_buttons:
		cell_buttons[button].color = null
		cell_buttons[button].icon = HexCellButton.Icon.None


