@tool
extends Button

@export var preview := Texture2D.new() :
    set(value):
        %TilePreview.texture = value

var rule : HexMapTileRule 

func set_rule(value: HexMapTileRule) -> void:
    rule = value

    if not is_node_ready():
        await ready

    %RuleId.text = str("(", rule.id, ")")

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    pass
