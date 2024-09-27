@tool
extends Node3D

@export var hex_map: HexMap

# OperatorEvaluatorEqualObject::evaluate()
#	* 
# godot::RefCounted does not pass operator= down to underlying type

func _ready():
	# print("HexMap baked meshes ", hex_map.get_bake_meshes())
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _on_hex_map_cells_changed(cells: Array) -> void:
	print("Sandbox HexMap emitted `cells_changed`: ", cells)
