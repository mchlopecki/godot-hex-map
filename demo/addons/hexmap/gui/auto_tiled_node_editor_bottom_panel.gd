@tool
extends VBoxContainer

# XXX this may the the route we need to go in, expose the node fully so we can
# do everything with minimal fuckery with the c++ code
@export var node: HexMapAutoTiled :
	set(value) :
		if value == node:
			return

		if node != null:
			node.mesh_library_changed.disconnect(_on_mesh_library_changed)
			node.rules_changed.disconnect(_on_rules_changed)

		node = value
		int_node = node.get_parent()
		node.mesh_library_changed.connect(_on_mesh_library_changed)
		node.rules_changed.connect(_on_rules_changed)
		_on_mesh_library_changed()
		_on_rules_changed()


var int_node: HexMapInt

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
	%RuleEditor.cell_scale = int_node.get_space().cell_scale
	%RuleEditor.cell_types = int_node.cell_types
	%RuleEditor.mesh_library = node.mesh_library
	%RuleEditor.visible = true
	%RuleEditor.set_rule(HexMapTileRule.new(), false)

func _on_rule_editor_save(rule: HexMapTileRule, is_update: bool):
	if is_update:
		node.update_rule(rule)
	else:
		node.add_rule(rule)

func _on_rule_editor_cancel():
	print("reset rule editor")
	pass

func _on_mesh_library_changed():
	%RuleEditor.mesh_library = node.mesh_library

func _on_rules_changed():
	print("rules changed")
	var rules = node.get_rules()
	var order = node.get_rules_order()
	for id in order:
		print("show rule[", id, "]: ", rules[id])
