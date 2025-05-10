@tool

extends Control

enum Tool { PAINT, ERASE, SELECT }
enum DisplayMode { THUMBNAIL, LIST }

signal mesh_changed

@export var mesh_library : MeshLibrary :
	set(value):
		if mesh_library != value:
			mesh_library = value
			update_mesh_list()

var mesh_id := -1

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%FilterLineEdit.text_changed.connect(set_mesh_filter)
	%PaintButton.pressed.connect(set_tool.bind(Tool.PAINT))
	%EraseButton.pressed.connect(set_tool.bind(Tool.ERASE))
	%SelectButton.pressed.connect(set_tool.bind(Tool.SELECT))
	%CopyButton.set_disabled(true)
	%MoveButton.set_disabled(true)
	%CCWButton.pressed.connect(rotate_tile.bind(-1))
	%CWButton.pressed.connect(rotate_tile.bind(1))
	%ListButton.pressed.connect(set_display_mode.bind(DisplayMode.LIST))
	%ThumbnailButton.pressed.connect(set_display_mode.bind(DisplayMode.THUMBNAIL))
	%ThumbnailButton.set_pressed(true)
	%ZoomWidget.changed.connect(set_mesh_scale)
	%MeshItemList.item_selected.connect(set_mesh_by_index)

	set_display_mode(DisplayMode.THUMBNAIL)

func set_display_mode(mode: DisplayMode) -> void:
	var ed_scale := EditorInterface.get_editor_scale()
	match mode:
		DisplayMode.THUMBNAIL:
			%MeshItemList.icon_mode = ItemList.ICON_MODE_TOP
			%MeshItemList.fixed_icon_size = Vector2i(64, 64) * ed_scale
			%ListButton.set_pressed(false)
		DisplayMode.LIST:
			%MeshItemList.icon_mode = ItemList.ICON_MODE_LEFT
			%MeshItemList.fixed_icon_size = Vector2i(64, 64) * ed_scale
			%ThumbnailButton.set_pressed(false)

	set_mesh_scale(%ZoomWidget.zoom_level)

func set_mesh_scale(scale: float) -> void:
	%MeshItemList.icon_scale = scale

func set_tool(tool: Tool) -> void:
	match tool:
		Tool.PAINT:
			%EraseButton.set_pressed(false)
			%SelectButton.set_pressed(false)
		Tool.ERASE:
			%PaintButton.set_pressed(false)
			%SelectButton.set_pressed(false)
			%MeshItemList.deselect_all()
			emit_signal("mesh_changed", -1)
		
		Tool.SELECT:
			%PaintButton.set_pressed(false)
			%EraseButton.set_pressed(false)

func rotate_tile(dir: int) -> void:
	# XXX implement this
	print("rotate tile: ", dir)

func set_mesh_filter(filter: String) -> void:
	update_mesh_list()

func update_mesh_list() -> void:
	%MeshItemList.clear()
	if mesh_library == null:
		%FilterLineEdit.text = ""
		%FilterLineEdit.set_editable(false)
		return

	%FilterLineEdit.set_editable(true)
	var filter = %FilterLineEdit.text.strip_edges()
	for id in mesh_library.get_item_list():
		name = mesh_library.get_item_name(id)
		if name.is_empty():
			name = "#%d" % id

		if !filter.is_empty() && !filter.is_subsequence_ofn(name):
			continue

		var icon = mesh_library.get_item_preview(id)
		var index = %MeshItemList.add_item(name, icon)
		%MeshItemList.set_item_tooltip(index, name)
		%MeshItemList.set_item_metadata(index, id)

	%MeshItemList.sort_items_by_text()

	pass

func set_mesh_by_index(index: int) -> void:
	if index == -1:
		mesh_id = -1
	else:
		mesh_id = %MeshItemList.get_item_metadata(index)
	emit_signal("mesh_changed", mesh_id)

func clear_selection() -> void:
	if mesh_id != -1:
		%MeshItemList.deselect_all()
		set_mesh_by_index(-1)
