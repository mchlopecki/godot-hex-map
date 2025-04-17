@tool
extends PanelContainer 

@export var rules := [] :
    set(value):
        rules = value
        call_deferred("_redraw_rules")
@export var mesh_library := MeshLibrary.new() :
    set(value):
        if value != mesh_library:
            mesh_library = value
            call_deferred("_redraw_rules")

signal new_rule_pressed()
signal rule_selected(rule: HexMapTileRule)

const rule_item: PackedScene = preload("auto_tiled_node_rule_list_item.tscn")

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    %NewRuleButton.pressed.connect(func(): new_rule_pressed.emit())
    pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    pass

func _redraw_rules() -> void:
    for child in %Rules.get_children():
        child.queue_free()

    for rule in rules:
        var control = rule_item.instantiate()
        control.pressed.connect(func(): rule_selected.emit(rule))
        control.set_rule(rule)
        control.preview = mesh_library.get_item_preview(rule.tile)
        %Rules.add_child(control)
        
    pass
