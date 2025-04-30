@tool
extends PanelContainer
class_name HexMapAutoTiledNodeRuleListItem

signal pressed

@export var preview := Texture2D.new() :
    set(value):
        if not is_node_ready():
            await ready
        preview = value
        %TilePreview.texture = value
        %Button.icon = value

@export var selected := false :
    set(value):
        selected = value
        _update_rule_panel_stylebox()

@export var hovered := false :
    set(value):
        hovered = value
        _update_rule_panel_stylebox()

var rule : HexMapTileRule 

func set_rule(value: HexMapTileRule) -> void:
    rule = value

    if not is_node_ready():
        await ready

    %RuleId.text = str("(", rule.id, ")")
    %Button.text = str("(", rule.id, ")")

func get_rule() -> HexMapTileRule:
    return rule

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    %Button.pressed.connect(func(): pressed.emit())
    %RulePanel.mouse_entered.connect(func(): hovered = true)
    %RulePanel.mouse_exited.connect(func(): hovered = false)
    pass # Replace with function body.

func _notification(what: int) -> void:
    if what == NOTIFICATION_DRAG_END:
        # All of our siblings get this signal when 
        remove_theme_stylebox_override("panel")

func _gui_input(event: InputEvent) -> void:
    # only select when we left-click within the %RulePanel; we don't want to
    # select the rule when the drag handle is clicked.
    if event is InputEventMouseButton \
            and event.is_pressed() \
            and event.button_index == MOUSE_BUTTON_MASK_LEFT \
            and %RulePanel.get_rect().has_point(event.position):
        selected = true
        pressed.emit()

func _get_drag_data(at_position: Vector2) -> Variant:
    if not %DragHandle.get_rect().has_point(at_position):
        return
    add_theme_stylebox_override("panel", get_theme_stylebox("drag"))
    return self

func _update_rule_panel_stylebox() -> void:
    var stylebox = null
    if selected:
        stylebox = get_theme_stylebox("focus")
    elif hovered:
        stylebox = get_theme_stylebox("hovered")
   
    if stylebox:
        %RulePanel.add_theme_stylebox_override("panel", stylebox)
    else:
        %RulePanel.remove_theme_stylebox_override("panel")
