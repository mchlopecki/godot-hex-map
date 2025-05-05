@tool
extends VBoxContainer

signal save_pressed(rule: HexMapTileRule, is_update: bool)
signal cancel_pressed

@export var cell_types := [
    { "value": 0, "name": "land", "color": Color.LAWN_GREEN },
    { "value": 1, "name": "sea", "color": Color.DEEP_SKY_BLUE },
    { "value": 2, "name": "river", "color": Color.LIGHT_BLUE },
] : 
    set(value):
        cell_types = value
        if not is_node_ready():
            await ready
        _on_cell_types_changed()

@export var mesh_library: MeshLibrary :
    set(value):
        mesh_library = value
        if not is_node_ready():
            await ready
        _on_mesh_library_changed()
        # %RulePainter3D.mesh_library = value
        # _rebuild_mesh_item_list()

# cell radius & height used for building cell meshes in the preview
@export var cell_scale := Vector3(1, 1, 1)

# rule being edited
var rule : HexMapTileRule

# set to true when we're updating an existing rule; XXX maybe pull this from
# rule.id == USHRT_MAX?
var is_update: bool

# map cell type to color, built from cell_types in _on_cell_types_changed()
var cell_type_colors := {}

var selected_type := -1

# hex map settings
var hex_space : HexMapSpace;

func set_hex_space(value: HexMapSpace) -> void:
    hex_space = value
    %RulePainter3D.set_hex_space(value)

func clear() -> void:
    %CellTypePalette.clear()
    %MeshPalette.clear()
    %RulePainter3D.reset()
    rule = HexMapTileRule.new()

func show_message(text: String) -> void:
    %Notification.text = "[center]" + text + "[/center]"

func set_rule(value: HexMapTileRule, update: bool) -> void:
    rule = value 

    # reset the painter
    %RulePainter3D.reset()
    %Notification.text = ""

    # update the painter cell state, and for _on_painter_cell_clicked, create
    # a dictionary to look up cell state by cell id
    for cell in rule.get_cells():
        %RulePainter3D.set_cell(cell["offset"],
            [cell["state"], cell_type_colors[cell["type"]]])
    _on_mesh_palette_selected(value.tile)

    is_update = update

func set_cell_state(offset: HexMapCellId, state: String, type) -> void:
    # if a type was specified, look up the color for that type
    var color = null
    if type != null:
        color = cell_type_colors[type].color

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

func focus_painter() -> void:
    %RulePainter3D.grab_focus()

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    %CancelButton.pressed.connect(func(): cancel_pressed.emit())
    %SaveButton.pressed.connect(func(): save_pressed.emit(rule, is_update))
    %RulePainter3D.focus_entered.connect(func(): %ActiveBorder.visible = true)
    %RulePainter3D.focus_exited.connect(func(): %ActiveBorder.visible = false)
    %RulePainter3D.cell_clicked.connect(_on_painter_cell_clicked)
    %RulePainter3D.layer_changed.connect(_on_painter_layer_changed)
    %CellTypePalette.selected.connect(_on_cell_type_palette_selected)
    %MeshPalette.selected.connect(_on_mesh_palette_selected)

    var layer = 2
    for child in %LayerSelector.get_children():
        child.pressed.connect(_on_layer_select.bind(child, layer))
        layer -= 1

    _on_cell_types_changed()
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

func _on_cell_types_changed() -> void:
    cell_type_colors.clear()
    cell_type_colors[null] = null
    cell_type_colors[-1] = null
    %CellTypePalette.clear()
    for type in cell_types:
        cell_type_colors[type.value] = type.color
        %CellTypePalette.add_item({
            "id": type.value,
            "preview": type.color,
            "desc": type.name,
        })
    %RulePainter3D.cell_types = cell_types

func _on_mesh_library_changed() -> void:
    %MeshPalette.clear()
    for id in mesh_library.get_item_list():
        %MeshPalette.add_item({
            "id": id,
            "preview": mesh_library.get_item_preview(id),
            "desc": mesh_library.get_item_name(id),
        })

func _on_cell_type_palette_selected(id: int) -> void:
    selected_type = id
    pass

func _on_mesh_palette_selected(id: int) -> void:
    if rule == null:
        return
    rule.tile = id
    if id != -1:
        %RulePainter3D.tile_mesh = mesh_library.get_item_mesh(id)
        %RulePainter3D.tile_mesh_transform = mesh_library \
                .get_item_mesh_transform(id)
    else:
        %RulePainter3D.tile_mesh = null
        %RulePainter3D.tile_mesh_transform = Transform3D()

func _on_painter_cell_clicked(cell_id: HexMapCellId, button: int, drag: bool) -> void:
    var cell = rule.get_cell(cell_id)
    if !cell:
        return

    var color = cell_type_colors[selected_type]
    # check for painting click, only when a cell type is selected in the
    # palette
    if button == MOUSE_BUTTON_MASK_LEFT && selected_type != -1:
        if not drag \
                and cell.state == "type" \
                and cell.type == selected_type:
            rule.set_cell_type(cell_id, selected_type, true)
            %RulePainter3D.set_cell(cell_id, ["not_type", color])
        else:
            rule.set_cell_type(cell_id, selected_type)
            %RulePainter3D.set_cell(cell_id, ["type", color])

    # handle erase click
    if button == MOUSE_BUTTON_MASK_RIGHT:
        if cell.state == "disabled" and not drag:
            rule.set_cell_empty(cell_id)
            %RulePainter3D.set_cell(cell_id, ["empty"])
        elif cell.state == "empty" and not drag:
            rule.set_cell_empty(cell_id, true)
            %RulePainter3D.set_cell(cell_id, ["not_empty"])
        else:
            rule.clear_cell(cell_id)
            %RulePainter3D.set_cell(cell_id, ["disabled"])

func _on_layer_select(node: TextureButton, layer: int):
    if node.button_pressed == false:
        node.button_pressed = true
        return
    %RulePainter3D.active_layer = layer

func _on_painter_layer_changed(layer: int):
    var count := 2
    for child in %LayerSelector.get_children():
        child.button_pressed = layer == count
        count -= 1
    
