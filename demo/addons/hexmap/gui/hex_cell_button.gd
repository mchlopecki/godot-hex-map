@tool
extends TextureButton

@export var color : Color :
	set(value):
		if value != color:
			color = value
			set_textures()

static var cell_template : Image = preload("../icons/hex_cell_template.png")
static var empty_textures := {};

const TEMPLATE_BORDER = 1
const TEMPLATE_BG = 2
const TEMPLATE_BG_STRIPE = 0

static func _static_init() -> void:
	# build the textures for an empty button
	var base = [
		TEMPLATE_BG, Color.from_ok_hsl(0, 0, 0.2, 1),
		TEMPLATE_BG_STRIPE, Color.from_ok_hsl(0, 0, 0.3, 1)
	]

	var normal := build_image(base + 
		[TEMPLATE_BORDER, Color.from_ok_hsl(0, 0, 0.5, 1)])
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

	image.resize(64, 64, Image.INTERPOLATE_LANCZOS)
	return image

func set_textures() -> void:
	print("set_textures ", color)
	if !color:
		texture_normal = empty_textures["normal"]
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
	set_textures()
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
