@tool
extends Control

static var cell_button: PackedScene = preload("hex_cell_button.tscn")
const cell_radius := 18
var cell_buttons := {}

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	set_size(Vector2(cell_radius * 10, 200) * EditorInterface.get_editor_scale())
	print("ready size ", get_size())
	var cells = HexMapCellId.new().get_neighbors(2, true)
	var node : HexMapInt = HexMapInt.new()
	var scale := EditorInterface.get_editor_scale();
	node.set_cell_radius(cell_radius * scale)
	for cell_id in cells:
		if cell_id.get_y() != 0:
			continue
		var center3 = node.get_cell_center(cell_id)
		var center = Vector2(center3.x, center3.z) + get_size()/2
		var button = cell_button.instantiate()
		add_child(button)
		button.set_position(center)
		button.pressed.connect(cell_pressed.bind(cell_id))
		cell_buttons[cell_id.as_int()] = button
	pass

func _gui_input(event: InputEvent) -> void:
	if(event is InputEventMouse):
		pass
		# print(event)
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func cell_pressed(id: HexMapCellId) -> void:
	print("cell pressed ", id)
	set_cell_color(id, Color.PINK)

func set_cell_color(id: HexMapCellId, color: Color) -> void:
	print("buttons ", cell_buttons)
	var button = cell_buttons[id.as_int()]
	if !button:
		print("unknown cell_id ", id)
		return
	button.color = color
