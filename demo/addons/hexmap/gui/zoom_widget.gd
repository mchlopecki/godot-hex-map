@tool
extends HBoxContainer

signal changed

var zoom_level := 1.0 : set = set_zoom_level

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	%ZoomLabel.text = "%.1f%%" % (zoom_level * 100)
	%ZoomLabel.custom_minimum_size = Vector2i(60 * EditorInterface.get_editor_scale(), 0)
	%ZoomLabel.pressed.connect(set_zoom_level.bind(1.0))
	%ZoomInButton.pressed.connect(adjust_zoom_level.bind(0.2))
	%ZoomOutButton.pressed.connect(adjust_zoom_level.bind(-0.2))

func adjust_zoom_level(delta: float) -> void:
	zoom_level += delta

func set_zoom_level(value: float) -> void:
	zoom_level = value
	emit_signal("changed", zoom_level)
	%ZoomLabel.text = "%.1f%%" % (zoom_level * 100)
