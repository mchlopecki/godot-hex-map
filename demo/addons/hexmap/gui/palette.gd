@tool
extends VBoxContainer

signal selected(id)

@export var selected_id := -1 :
    set(value):
        selected_id = value
        %Filter.release_focus()
        rebuild_palette()
        selected.emit(value)
@export var selected_item : Dictionary :
    set(value):
        selected_id = value.id
    get():
        if items.has(selected_id):
            return items[selected_id]
        else:
            return {}

var items := {}
# faux button, displays a color in a square, and optionally highlights it with
# a border when selected.
class PaletteChip:
    extends Panel

    @export var preview : Texture2D :
        set(value):
            preview = value
            _update_chip()

    @export var color : Color :
        set(value):
            color = value
            _update_stylebox()
    @export var selected := false :
        set(value):
            selected = value
            _update_stylebox()

    signal pressed

    func _ready() -> void:
        _update_stylebox()

    func _update_stylebox():
        var stylebox = get_theme_stylebox("panel").duplicate()
        stylebox.bg_color = color

        if selected:
            stylebox.border_width_top = 5
            stylebox.border_width_left = 5
            stylebox.border_width_right = 5
            stylebox.border_width_bottom = 5
            stylebox.border_color = Color.WHITE
            stylebox.shadow_color = Color.BLACK
            stylebox.shadow_size = 5

        add_theme_stylebox_override("panel", stylebox)

    func _update_chip():
        if preview:
            var texture_rect := TextureRect.new()
            texture_rect.texture = preview
            add_child(texture_rect)

    func _gui_input(event: InputEvent) -> void:
        if event is InputEventMouseButton \
                and event.is_pressed() \
                and event.button_index == MOUSE_BUTTON_LEFT:
            pressed.emit()

class PaletteEntry:
    extends PanelContainer

    @export var preview : Control :
        set(value):
            preview = value
            _rebuild()
    @export var desc := "Description"
    @export var selected := false :
        set(value):
            selected = value
            _rebuild()

    signal pressed

    func _rebuild() -> void:
        if preview.get_parent():
            preview.get_parent().remove_child(preview)

        for child in get_children():
            remove_child(child)
            child.queue_free()
        remove_theme_stylebox_override("panel")

        var label = Label.new()
        label.text = desc

        var hbox = HBoxContainer.new()
        hbox.add_child(preview)
        hbox.add_child(label)
        add_child(hbox)
    
        var stylebox = get_theme_stylebox("panel").duplicate()
        if selected: 
            stylebox.bg_color = Color.hex(0x363D4Aff)
        else:
            stylebox.bg_color = Color.hex(0x262b33ff)

        add_theme_stylebox_override("panel", stylebox)

    func _gui_input(event: InputEvent) -> void:
        if event is InputEventMouseButton \
                and event.is_pressed() \
                and event.button_index == MOUSE_BUTTON_LEFT:
            pressed.emit()

# clear all items from the palette
func clear() -> void:
    selected_id = -1
    items.clear()
    rebuild_palette()

func add_item(item: Dictionary) -> void:
    print("add_item ", item)
    print("item.preview is a texture ", item.preview is Texture2D)
    var id = item.id
    if items.has(id):
        push_error("item id already defined ", items[id],
                ", cannot add ", item)
        return

    items[id] = item

    rebuild_palette()

func filtered_item_keys() -> Array:
    var keys = items.keys()

    var filter = %Filter.text
    if not filter.is_empty():
        keys = keys.filter(func(key):
            return items[key].desc.containsn(filter)
        )

    keys.sort_custom(func(a, b):
        return items[a]["desc"] < items[b]["desc"]
    )

    return keys
    
func rebuild_palette() -> void:
    for child in %CompactPalette.get_children():
        %CompactPalette.remove_child(child)
        child.queue_free()
    for child in %FullPalette.get_children():
        %FullPalette.remove_child(child)
        child.queue_free()

    %CompactPalette.visible = false
    %FullPalette.visible = false

    if %CompactViewButton.button_pressed:
        populate_compact_palette()
    else:
        populate_full_palette()

func populate_compact_palette() -> void:

    %CompactPalette.visible = true 

    for id in filtered_item_keys():
        var item = items[id]
        var button
        if item.preview is Color:
            button = PaletteChip.new()
            button.color = item.preview
            button.selected = id == selected_id
            button.custom_minimum_size = Vector2(64, 64)
            button.tooltip_text = item.desc
        elif item.preview is Texture2D:
            button = PaletteChip.new()
            button.preview = item.preview
            button.color = Color.TRANSPARENT
            button.selected = id == selected_id
            button.custom_minimum_size = Vector2(64, 64)
            button.tooltip_text = item.desc
        else:
            push_error("invalid type for item.preview: ", item)

        button.pressed.connect(func(): selected_id = id)
        %CompactPalette.add_child(button)

func populate_full_palette() -> void:
    %FullPalette.visible = true 

    for id in filtered_item_keys():
        var item = items[id]
        var preview 
        if item.preview is Color:
            preview = ColorRect.new()
            preview.color = item.preview
            preview.custom_minimum_size = Vector2(64, 64)
        elif item.preview is Texture2D:
            preview = TextureRect.new()
            preview.texture = item.preview
            preview.custom_minimum_size = Vector2(64, 64)
        else:
            push_error("invalid type for item.preview: ", item)

        var entry = PaletteEntry.new()
        entry.preview = preview
        entry.desc = item.desc
        entry.selected = id == selected_id
        entry.pressed.connect(func(): selected_id = id)

        %FullPalette.add_child(entry)

    pass
    

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    %CompactViewButton.pressed.connect(func(): rebuild_palette())
    %Filter.text_changed.connect(func(t): rebuild_palette())
    rebuild_palette()
    pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    pass

func _on_button_pressed() -> void:
    print("button pressed")
