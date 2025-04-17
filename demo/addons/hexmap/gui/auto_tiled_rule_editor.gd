@tool
extends VBoxContainer

signal save_pressed(rule: HexMapTileRule, is_update: bool)
signal cancel_pressed

@export var cell_types: Array :
	set(value):
		if not is_node_ready():
			await ready
		%TypeSelector.cell_types = value
		cell_types = value
		cell_types_by_value.clear()
		for type in cell_types:
			cell_types_by_value[type.value] = type
			

@export var mesh_library: MeshLibrary :
	set(value):
		mesh_library = value
		_rebuild_mesh_item_list()

# cell radius & height used for building cell meshes in the preview
@export var cell_scale := Vector3(1, 1, 1)

var rule : HexMapTileRule
var is_update: bool
var cell_types_by_value: Dictionary

func clear() -> void:
	%GridPainter.reset()
	%MeshItemList.clear()
	%TileMeshInstance3D.mesh = _build_cell_mesh()
	rule = HexMapTileRule.new()

func set_rule(value: HexMapTileRule, update: bool) -> void:
	rule = value 
	for cell in rule.get_cells():
		print("cell ", cell)
		set_cell_state(cell["offset"], cell["state"], cell["type"])
		pass

	var mesh = mesh_library.get_item_mesh(value.tile)
	%TileMeshInstance3D.mesh = mesh

	is_update = update

func set_cell_state(offset: HexMapCellId, state: String, type) -> void:
	# if a type was specified, look up the color for that type
	var color = null
	if type != null:
		color = cell_types_by_value[type].color

	# print("set_cell_state() ", state, ", type ", type, ", color ", color)
	match state:
		"empty":
			rule.set_cell_empty(offset)
			%GridPainter.set_cell(offset, null, "not")
		"not_empty":
			rule.set_cell_empty(offset, true)
			%GridPainter.set_cell(offset, Color.WEB_GRAY, "some")
		"type":
			rule.set_cell_type(offset, type)
			%GridPainter.set_cell(offset, color, null)
		"not_type":
			rule.set_cell_type(offset, type, true)
			%GridPainter.set_cell(offset, color, "not")
		"disabled":
			rule.clear_cell(offset)
			%GridPainter.set_cell(offset, null, null)
		_:
			push_error("unsupported cell state ", state)

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%GridPainter.cell_clicked.connect(_on_painter_cell_clicked)
	%MeshItemList.item_selected.connect(_on_mesh_item_selected)
	%CancelButton.pressed.connect(func(): cancel_pressed.emit())
	%SaveButton.pressed.connect(func(): save_pressed.emit(rule, is_update))
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _rebuild_mesh_item_list() -> void:
	%MeshItemList.clear()

	for id in mesh_library.get_item_list():
		var name = mesh_library.get_item_name(id)
		if name.is_empty():
			name = "id_%d" % id
		var icon = mesh_library.get_item_preview(id)
		var index = %MeshItemList.add_item(name, icon)
		%MeshItemList.set_item_tooltip(index, name)
		%MeshItemList.set_item_metadata(index, id)

	%MeshItemList.sort_items_by_text()

func _on_mesh_item_selected(index: int):
	var id = %MeshItemList.get_item_metadata(index)
	var mesh = mesh_library.get_item_mesh(id)
	%TileMeshInstance3D.mesh = mesh
	rule.tile = id

func _on_painter_cell_clicked(id: HexMapCellId, button: MouseButton):
	var cell = rule.get_cell(id)
	var color = %TypeSelector.selected_color
	var type = %TypeSelector.selected_type

	if button == MOUSE_BUTTON_LEFT:
		if cell["state"] == "type" && cell["type"] == type:
			set_cell_state(id, "not_type", type)
		else:
			set_cell_state(id, "type", type)
	elif button == MOUSE_BUTTON_RIGHT:
		if cell["state"] == "disabled":
			set_cell_state(id, "empty", null)
		elif cell["state"] == "empty":	
			set_cell_state(id, "not_empty", null)
		else:
			set_cell_state(id, "disabled", null)

func _build_cell_mesh() -> Mesh:
	var mesh := CylinderMesh.new()
	mesh.height = cell_scale.y
	mesh.top_radius = cell_scale.x
	mesh.bottom_radius = 1
	mesh.radial_segments = 6
	mesh.rings = 1
	return mesh
