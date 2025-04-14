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

@export var mesh_library: MeshLibrary :
	set(value):
		mesh_library = value
		_rebuild_mesh_item_list()

# cell radius & height used for building cell meshes in the preview
@export var cell_scale := Vector3(1, 1, 1)

var rule : HexMapTileRule
var is_update: bool

func clear() -> void:
	%GridPainter.reset()
	%MeshItemList.clear()
	%TileMeshInstance3D.mesh = _build_cell_mesh()
	rule = HexMapTileRule.new()

func set_rule(rule: HexMapTileRule, update: bool) -> void:
	rule = rule
	is_update = update

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
	print("rule updated ", rule)

func _on_painter_cell_clicked(id: HexMapCellId, button: MouseButton):
	var cell = rule.get_cell(id)
	var color = %TypeSelector.selected_color
	var type = %TypeSelector.selected_type

	if button == MOUSE_BUTTON_LEFT:
		if cell["state"] == "type" && cell["type"] == type:
			rule.set_cell_type(id, type, true)
			%GridPainter.set_cell_color(id, color)
			%GridPainter.set_cell_not(id, true)
		else:
			rule.set_cell_type(id, type)
			%GridPainter.set_cell_color(id, color)
			%GridPainter.set_cell_not(id, false)
	elif button == MOUSE_BUTTON_RIGHT:
		%GridPainter.set_cell_color(id, null)
		if cell["state"] == "disabled":
			rule.set_cell_empty(id)
			%GridPainter.set_cell_not(id, true)
		elif cell["state"] == "empty":	
			# XXX need some visualization for must be some value
			rule.set_cell_empty(id, true)
			%GridPainter.set_cell_not(id, false)
		else:
			rule.clear_cell(id)
			%GridPainter.set_cell_not(id, false)

func _build_cell_mesh() -> Mesh:
	var mesh := CylinderMesh.new()
	mesh.height = cell_scale.y
	mesh.top_radius = cell_scale.x
	mesh.bottom_radius = 1
	mesh.radial_segments = 6
	mesh.rings = 1
	return mesh
