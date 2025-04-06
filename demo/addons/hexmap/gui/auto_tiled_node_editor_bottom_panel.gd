@tool
extends VBoxContainer

@export var hex_space: HexMapSpace :
	set(value):
		print("set hex_space ", value.cell_scale)
		hex_space = value
@export var cell_types: Array
@export var mesh_library: MeshLibrary
@export var cell_radius: float = 1.0 :
	set(value) :
		cell_radius = value
		%RuleEditor.cell_radius = value
	
@export var cell_height: float = 1.0 :
	set(value) :
		cell_height = value
		%RuleEditor.cell_height = value

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%NewRuleButton.pressed.connect(_on_new_rule_pressed)
	%RuleEditor.save_pressed.connect(_on_rule_editor_save)
	%RuleEditor.cancel_pressed.connect(_on_rule_editor_cancel)

	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _on_new_rule_pressed():
	%RuleEditor.clear()
	%RuleEditor.cell_types = cell_types
	%RuleEditor.mesh_library = mesh_library
	%RuleEditor.visible = true

func _on_rule_editor_save():
	print("save rule editor")
	pass

func _on_rule_editor_cancel():
	print("reset rule editor")
	pass
