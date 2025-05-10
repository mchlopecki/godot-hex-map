@tool
extends VBoxContainer

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


# local copy of the AutoTiledNode's parent IntNode
var int_node: HexMapInt

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%NewRuleButton.pressed.connect(_on_new_rule_pressed)

	%RulesList.rule_selected.connect(_on_rule_selected)
	%RulesList.order_changed.connect(_on_rules_order_changed)
	%RulesList.delete_rule.connect(_on_rules_list_delete)
	%RulesList.rule_changed.connect(_on_rules_list_rule_changed)

	%RuleEditor.save_pressed.connect(_on_rule_editor_save)
	%RuleEditor.cancel_pressed.connect(_on_rule_editor_cancel)
	%HSplitContainer.split_offset *= EditorInterface.get_editor_scale()

func edit_rule(rule: HexMapTileRule) -> void:
	%RulesList.selected_id = rule.id
	%Directions.visible = false
	%RuleEditor.clear()

	%RuleEditor.set_hex_space(int_node.get_space())
	%RuleEditor.cell_scale = int_node.get_space().cell_scale
	%RuleEditor.cell_types = int_node.cell_types
	%RuleEditor.mesh_library = node.mesh_library
	if rule.id == HexMapTileRule.ID_NOT_SET:
		%RuleEditor.mode = "Add"
	else:
		%RuleEditor.mode = "Update"
	%RuleEditor.set_rule(rule)
	%RuleEditor.visible = true
	%RuleEditor.focus_painter()

func _on_new_rule_pressed():
	edit_rule(HexMapTileRule.new())

func _on_rule_selected(rule: HexMapTileRule) -> void:
	edit_rule(rule)

func _on_rules_order_changed(value: Array) -> void:
	node.set_rules_order(value)

func _on_rule_editor_save(rule: HexMapTileRule):
	if %RuleEditor.mode == "Update":
		node.update_rule(rule)
		edit_rule(rule)
		%RuleEditor.show_message("[b]Saved rule " + str(rule.id) + "[/b]")
	else:
		var id = node.add_rule(rule)
		%RulesList.selected_id = id

		# update the rule id, and reopen the editor so it switches to edit mode
		rule.id = id
		edit_rule(rule)
		%RuleEditor.show_message("[b]Added rule " + str(id) + "[/b]")

func _on_rule_editor_cancel():
	%RuleEditor.clear()
	%RulesList.selected_id = -1
	%RuleEditor.visible = false
	%Directions.visible = true
	pass

func _on_mesh_library_changed():
	%RuleEditor.mesh_library = node.mesh_library
	%RulesList.mesh_library = node.mesh_library

func _on_rules_changed():
	%RulesList.rules = node.get_rules()
	%RulesList.order = node.get_rules_order()

func _on_rules_list_delete(id: int):
	# close out the editor if we're deleting the rule being edited
	if %RulesList.selected_id == id:
		_on_rule_editor_cancel()
	node.delete_rule(id)

func _on_rules_list_rule_changed(rule: HexMapTileRule):
	node.update_rule(rule)
