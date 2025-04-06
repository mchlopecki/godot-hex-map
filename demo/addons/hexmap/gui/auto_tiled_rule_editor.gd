@tool
extends VBoxContainer

signal save_pressed
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

# cell radius, used for building cell meshes in the preview
@export var cell_radius := 1.0
# cell height, used for building cell meshes in the preview
@export var cell_height := 1.0

func clear() -> void:
	%GridPainter.reset()
	%MeshItemList.clear()
	%TileMeshInstance3D.mesh = _build_cell_mesh()

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%GridPainter.cell_clicked.connect(_on_painter_cell_clicked)
	%MeshItemList.item_selected.connect(_on_mesh_item_selected)
	%CancelButton.pressed.connect(func(): cancel_pressed.emit())
	%SaveButton.pressed.connect(func(): save_pressed.emit())
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

func _on_painter_cell_clicked(id: HexMapCellId, button: MouseButton):
	var color = %TypeSelector.selected_color

	if button == MOUSE_BUTTON_LEFT:
		%GridPainter.set_cell_color(id, color)
	elif button == MOUSE_BUTTON_RIGHT:
		%GridPainter.set_cell_color(id, null)

func _build_cell_mesh() -> Mesh:
	var mesh := CylinderMesh.new()
	mesh.height = cell_height
	mesh.top_radius = cell_radius
	mesh.bottom_radius = cell_radius
	mesh.radial_segments = 6
	mesh.rings = 1
	return mesh
