@tool

extends VBoxContainer

# emitted when user selects a cell type
signal type_selected()
# emitted when the edit plane axis or depth is changed
signal edit_plane_changed(axis: int, depth: int)
# emitted to rotate cursor (-1 clockwise, 1 counter-clockwise)
signal rotate_cursor(step: int)
# emitted to move selection
signal move_selection
# emitted to clone selection
signal clone_selection
# emitted to clear selected tiles
signal clear_selection
# emitted to fill the selected tiles
signal fill_selection
# emitted when we want to set visibility of tile value visualization
signal set_tiled_map_visibility(visible: bool)

@export var editor_plugin: EditorPlugin
@export var node: HexMapInt:
    set(value):
        if node != null:
            node.cell_types_changed.disconnect(_on_node_cell_types_changed)
        node = value
        node.cell_types_changed.connect(_on_node_cell_types_changed)
        if is_node_ready():
            _on_node_cell_types_changed()

@export var editor_state := {
        "cursor_mesh_index": -1,
        "edit_plane_axis": 0,
        "edit_plane_depth": 0,
        "selection_active": false,
        "cursor_active": false, } :
    set(value):
        # XXX Need to handle cursor_mesh_index being changed from EditorPlugin
        # and updating the GUI without causing a signal loop.
        _on_cell_type_pressed(value.cursor_mesh_index)
        %EditPlaneSelector.axis = value.edit_plane_axis
        %EditPlaneSelector.depth = value.edit_plane_depth
        %CCWButton.disabled = !value.cursor_active
        %CWButton.disabled = !value.cursor_active
        %CopyButton.disabled = !value.selection_active
        %MoveButton.disabled = !value.selection_active
        %ClearButton.disabled = !value.selection_active
        %FillButton.disabled = !(value.cursor_active && value.selection_active)
        editor_state = value
        
@export var show_cells: bool = true :
    set(value):
        %ShowCellsButton.button_pressed = value

const rule_item: PackedScene = preload("int_node_cell_type_control.tscn")
const add_icon: CompressedTexture2D = preload("../icons/Add.svg")
const save_icon: CompressedTexture2D = preload("../icons/Save.svg")

enum EditMode { Add, Update }
var edit_mode: EditMode :
    set(value):
        edit_mode = value
        match value:
            EditMode.Add:
                %SubmitButton.text = "Create"
                %SubmitButton.icon = add_icon
                %DeleteButton.visible = false
                %DeleteConfirmButton.visible = false
                pass
            EditMode.Update:
                %SubmitButton.text = "Save"
                %SubmitButton.icon = save_icon
                %DeleteButton.visible = true
                %DeleteConfirmButton.visible = false
                pass

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    %EditPlaneSelector.changed.connect(func(axis: int, depth: int): edit_plane_changed.emit(axis, depth))
    %CCWButton.pressed.connect(rotate_cursor.emit.bind(1))
    %CWButton.pressed.connect(rotate_cursor.emit.bind(-1))
    %MoveButton.pressed.connect(move_selection.emit)
    %CopyButton.pressed.connect(clone_selection.emit)
    %FillButton.pressed.connect(fill_selection.emit)
    %ClearButton.pressed.connect(clear_selection.emit)
    %ShowCellsButton.toggled.connect(func(v): set_tiled_map_visibility.emit(v))

    %FilterLineEdit.text_changed.connect(_on_filter_text_changed)

    %SubmitButton.pressed.connect(_on_submit_button_pressed)
    %CancelButton.pressed.connect(_on_cancel_edit_pressed)
    %DeleteButton.pressed.connect(_on_delete_type_pressed)
    %DeleteConfirmButton.pressed.connect(_on_delete_type_confirmed)
    %CellTypeNameLineEdit.text_submitted \
        .connect(func(v): _on_submit_button_pressed())

    _on_node_cell_types_changed()

func _on_node_cell_types_changed() -> void:
    for child in %ColumnWrapContainer.get_children():
        %ColumnWrapContainer.remove_child(child)
        child.queue_free()

    # Insert the Add Cell Type button at the start of the list
    var add_button := Button.new()
    add_button.text = "Add Cell Type"
    add_button.icon = add_icon
    add_button.focus_mode = FOCUS_NONE
    add_button.pressed.connect(_on_add_cell_type_button_pressed)
    %ColumnWrapContainer.add_child(add_button)

    # grab the cell types and sort them by name
    var cell_types = node.get_cell_types()
    cell_types.sort_custom(func(a, b): return a.name < b.name)

    for type in cell_types:
        var child := rule_item.instantiate()
        child.value = type.value
        child.desc = type.name
        child.color = type.color
        child.pressed.connect(_on_cell_type_pressed.bind(type.value))
        child.pressed_edit.connect(
                _on_cell_type_pressed_edit.bind(type))

        %ColumnWrapContainer.add_child(child)

func _cell_type_nodes() -> Array:
    return %ColumnWrapContainer.get_children() \
        .filter(func(n): return n is not Button)

func _on_filter_text_changed(filter: String) -> void:
    var selected = editor_state.cursor_mesh_index
    var selected_visible = false
    if filter.is_empty():
        selected_visible = true
        for child in _cell_type_nodes():
            child.visible = true
    else:
        for child in _cell_type_nodes():
            child.visible = child.desc.containsn(filter)
            if child.visible && child.value == selected:
                selected_visible = true
    if selected != -1 and selected_visible == false:
        _on_cell_type_pressed(-1)

func _on_cell_type_pressed(value: int) -> void:
    for child in _cell_type_nodes():
        child.selected = child.value == value

    if editor_state.cursor_mesh_index != value:
        editor_state.cursor_mesh_index = value
        type_selected.emit()
    # XXX something isn't working right here when we hit escape in the editor
    # to clear the active tile.
    pass

func _get_popup_position() -> Vector2:
    var viewport = get_viewport()
    var viewport_rect := viewport.get_visible_rect()
    var pos = viewport.get_mouse_position()
    pos.y -= (%EditorPopup.get_contents_minimum_size().y/2)
    pos.x += 16
    var popup_size = %EditorPopup.get_contents_minimum_size()

    # make sure the popup isn't clipped
    if pos.y + popup_size.y > viewport_rect.size.y:
        pos.y = viewport_rect.size.y - popup_size.y

    return pos

func _on_add_cell_type_button_pressed() -> void:
    edit_mode = EditMode.Add

    # populate editor
    %CellTypeId.text = ""
    %CellTypeNameLineEdit.text = ""
    # random color
    %CellTypeColorPicker.color = Color.from_hsv(
			randf(),
			randf_range(0.2, 0.6),
			randf_range(0.9, 1.0))
    %CellTypeNameLineEdit.grab_focus()

    %EditorPopup.position = _get_popup_position()
    %EditorPopup.show()

func _on_cell_type_pressed_edit(type: Dictionary) -> void:
    edit_mode = EditMode.Update

    # populate editor with current type values
    %CellTypeId.text = str(type.value)
    %CellTypeNameLineEdit.text = type.name
    %CellTypeColorPicker.color = type.color

    %EditorPopup.position = _get_popup_position()
    %EditorPopup.show()

func _on_cancel_edit_pressed() -> void:
    %EditorPopup.hide()

func _on_submit_button_pressed() -> void:
    var id = int(%CellTypeId.text)
    var name = %CellTypeNameLineEdit.text
    var color = %CellTypeColorPicker.color

    var undo_redo := editor_plugin.get_undo_redo()
    match edit_mode:
        EditMode.Add:
            # We need to create the cell type outside of undo/redo to get an id
            # to remove during undo.
            id = node.set_cell_type(HexMapInt.TypeIdNext, name, color)

            # create an undo/redo for the new value
            undo_redo.create_action(
                str("HexMapInt: add cell type: ", name, " (", id, ")"))
            undo_redo.add_do_method(node, "set_cell_type", id, name, color)
            undo_redo.add_undo_method(node, "remove_cell_type", id)
            undo_redo.commit_action()
        EditMode.Update:
            # get the original values for the type
            var orig = node.get_cell_type(id)
            if not orig:
                push_error("unable to find original values for cell type ", id)
                return

            # add commit undo/redo action for setting the new values
            undo_redo.create_action(
                str("HexMapInt: update cell type: ", name, " (", id, ")"))
            undo_redo.add_do_method(node, "set_cell_type", id, name, color)
            undo_redo.add_undo_method(node, "set_cell_type", id,
                    orig.name, orig.color)
            undo_redo.commit_action()
            pass
            
    %EditorPopup.hide()

func _on_delete_type_pressed() -> void:
    %DeleteButton.visible = false
    %DeleteConfirmButton.visible = true

func _on_delete_type_confirmed() -> void:
    var id = int(%CellTypeId.text)
    var type = node.get_cell_type(id)
    if not type:
        push_error("unable to find original values for cell type ", id)
        return

    # clear the selected type if that's the one we're deleting; otherwise the
    # editor cursor will have an invalid mesh reference.
    if id == editor_state.cursor_mesh_index:
        _on_cell_type_pressed(-1)

    # find the cells in the int node that have this value
    var cell_vecs := node.find_cell_vecs_by_value(id)

    # look up the original cell states for undo
    var restore_cells := node.get_cells(cell_vecs)

    # create the args for set_cells() to clear all cells with this value
    var clear_cells := []
    clear_cells.resize(cell_vecs.size() * HexMapNode.CellArrayWidth)
    for i in range(cell_vecs.size()):
        var base = i * node.CellArrayWidth
        clear_cells[base] = cell_vecs[i]
        clear_cells[base + 1] = node.INVALID_CELL_VALUE

    # build the undo/redo action and commit it
    var undo_redo := editor_plugin.get_undo_redo()
    undo_redo.create_action(
            str("HexMapInt: remove cell type: ", type.name, " (", id, ")"))
    undo_redo.add_do_method(node, "set_cells", clear_cells)
    undo_redo.add_do_method(node, "remove_cell_type", id)
    undo_redo.add_undo_method(node,
            "set_cell_type", id, type.name, type.color)
    undo_redo.add_undo_method(node, "set_cells", restore_cells)
    undo_redo.commit_action()

    # hide the UI
    %EditorPopup.hide()
