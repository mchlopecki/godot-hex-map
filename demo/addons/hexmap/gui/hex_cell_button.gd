@tool
extends TextureButton

@export var color = null :
	set(value):
		if value != color:
			color = value
			update_textures()
@export var border_color = null :
	set(value):
		if value != border_color:
			border_color = value
			update_textures()
		

static var cell_template : Image = preload("../icons/hex_cell_template.png")
static var empty_textures := {};

# size of the button; scaled for hidpi devices
const IMAGE_SIZE = Vector2(32, 32)

# color channel used by each part of the template
const TEMPLATE_BORDER = 1
const TEMPLATE_BG = 2
const TEMPLATE_BG_STRIPE = 0

# colors used for empty cells
static var EMPTY_BG_COLOR = Color.from_ok_hsl(0, 0, 0.2, 1)
static var EMPTY_BG_STRIPE_COLOR = Color.from_ok_hsl(0, 0, 0.3, 1)
static var EMPTY_BORDER_COLOR = Color.from_ok_hsl(0, 0, 0.5, 1)

static func _static_init() -> void:
	# build the textures for an empty button
	var base = [
		TEMPLATE_BG, EMPTY_BG_COLOR,
		TEMPLATE_BG_STRIPE, EMPTY_BG_STRIPE_COLOR,
	]

	var normal := build_image(base + [TEMPLATE_BORDER, EMPTY_BORDER_COLOR])
	empty_textures["normal"] = ImageTexture.create_from_image(normal)

	var bitmap := BitMap.new()
	bitmap.create_from_image_alpha(normal)
	empty_textures["click_mask"] = bitmap

	empty_textures["hover"] = ImageTexture.create_from_image(
		build_image(base + [ TEMPLATE_BORDER, Color.GOLD ])
	)
	empty_textures["pressed"] = ImageTexture.create_from_image(
		build_image(base + [ TEMPLATE_BORDER, Color.WHITE, ]))

# build a hex cell Image, coloring various parts of the template image as
# requested
static func build_image(steps: Array) -> Image:
	var size = cell_template.get_size()

	var image := Image.create(size.x, size.y, false, Image.FORMAT_RGBA8)
	image.fill(Color.from_hsv(0,0,0,0))
	image.set_pixel(size.x -1, size.y -1, Color.CORAL)
	image.resize(size.x, size.y)
	for i in range(0, steps.size(), 2):
		var channel = steps[i]
		var color = steps[i+1]
	
		for x in range(size.x):
			for y in range(size.y):
				var adj = cell_template.get_pixel(x, y)[channel]
				if adj > 0.5:
					image.set_pixel(x, y, color * adj)
				pass

	# resize the image down to the size we need
	# we're doing all this work at a higher resolution then scaling to reduce
	# visual artifacts from manually setting the pixels that have multiple
	# channels set.
	size = IMAGE_SIZE * EditorInterface.get_editor_scale()
	image.resize(size.x, size.y, Image.INTERPOLATE_LANCZOS)
	return image

func update_textures() -> void:
	if !color && !border_color:
		texture_normal = empty_textures["normal"]
		texture_hover = empty_textures["hover"]
		texture_pressed = empty_textures["pressed"]
		texture_click_mask = empty_textures["click_mask"]
	elif !color:
		# we only have a border color, so build the normal image only
		var normal := build_image([
			TEMPLATE_BG, EMPTY_BG_COLOR,
			TEMPLATE_BG_STRIPE, EMPTY_BG_STRIPE_COLOR,
			TEMPLATE_BORDER, border_color
		])
		texture_normal = ImageTexture.create_from_image(normal)
		texture_hover = empty_textures["hover"]
		texture_pressed = empty_textures["pressed"]
		texture_click_mask = empty_textures["click_mask"]
	else:
		var base := [TEMPLATE_BG, color]
		var normal := build_image(base + 
			[TEMPLATE_BORDER, Color.from_ok_hsl(0, 0, 0.5, 1)])
		texture_normal = ImageTexture.create_from_image(normal)

		var bitmap := BitMap.new()
		bitmap.create_from_image_alpha(normal)
		texture_click_mask = bitmap

		texture_hover = ImageTexture.create_from_image(
			build_image(base + [ TEMPLATE_BORDER, Color.GOLD ])
		)

		texture_pressed = ImageTexture.create_from_image(
			build_image(base + [ TEMPLATE_BORDER, Color.WHITE, ]))

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	update_textures()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _get_minimum_size() -> Vector2:
	return IMAGE_SIZE * EditorInterface.get_editor_scale()

