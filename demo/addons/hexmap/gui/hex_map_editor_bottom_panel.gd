@tool

extends Control

enum Tool { PAINT, ERASE, SELECT }
enum DisplayMode { THUMBNAIL, LIST }

@export var mesh_library : MeshLibrary :
    set(value):
        if mesh_library == value:
            return

        mesh_library = value
        update_mesh_list()
@export var editor_plugin: EditorPlugin :
    set(value):
        if editor_plugin != null:
            # disconnect existing EditorPlugin
            editor_plugin.paint_value_changed \
                    .connect(_on_editor_plugin_paint_value_changed)
            pass

        editor_plugin = value

        # set up all the needed connections
        editor_plugin.paint_value_changed \
                .connect(_on_editor_plugin_paint_value_changed)

        %CommonToolbar.editor_plugin = editor_plugin

var paint_value := -1 :
    set(value):
        if value == paint_value:
            return
        paint_value = value
        %MeshPalette.selected_id = value

var mesh_id := -1

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    %FilterLineEdit.text_changed.connect(func(v): %MeshPalette.filter = v)
    %MeshPalette.compact_view = %ThumbnailButton.button_pressed
    %ThumbnailButton.toggled.connect(func(v): %MeshPalette.compact_view = v)
    %MeshPalette.selected.connect(func(v): editor_plugin.paint_value = v)
    %ZoomWidget.changed.connect(_on_zoom_widget_changed)
    _on_zoom_widget_changed(1.0)

func set_mesh_filter(filter: String) -> void:
    update_mesh_list()

func update_mesh_list() -> void:
    %MeshPalette.clear()
    for id in mesh_library.get_item_list():
        %MeshPalette.add_item({
            "id": id,
            "preview": mesh_library.get_item_preview(id),
            "desc": mesh_library.get_item_name(id),
        })

func set_mesh_by_index(index: int) -> void:

    if index == -1:
        mesh_id = -1
    else:
        mesh_id = %MeshItemList.get_item_metadata(index)
    editor_plugin.paint_value = mesh_id

func clear_selection() -> void:
    if mesh_id != -1:
        %MeshItemList.deselect_all()
        set_mesh_by_index(-1)

func _on_zoom_widget_changed(scale: float) -> void:
    %MeshPalette.preview_size = Vector2(64, 64) * scale

func _on_editor_plugin_paint_value_changed(value: int) -> void:
    paint_value = value
