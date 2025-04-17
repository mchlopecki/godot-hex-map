@tool
extends TextureButton
class_name HexCellButton

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
@export var icon := Icon.None :
    set(value):
        if value != icon:
            print("setting icon to ", value)
            icon = value
            update_textures()

static var cell_template : Image = preload("../icons/hex_cell_template.png")
static var icon_templates := {
    Icon.X: preload("../icons/hex_cell_x.png"),
    Icon.Star: preload("../icons/hex_cell_star.png"),
}
static var empty_textures := {};

# size of the button; scaled for hidpi devices
const IMAGE_SIZE = Vector2(32, 32)

# color channel used by each part of the template
const TEMPLATE_CHANNEL_BORDER = 1
const TEMPLATE_CHANNEL_BG = 2
const TEMPLATE_CHANNEL_BG_STRIPE = 0

enum Template {
    COLOR, # <channel>, <color>
    ICON,
}

enum Icon {
    None,
    X,
    Star,
}

# colors used for empty cells
static var DEFAULT_BG_COLOR = Color.from_ok_hsl(0, 0, 0.2, 1)
static var DEFAULT_BG_STRIPE_COLOR = Color.from_ok_hsl(0, 0, 0.3, 1)
static var DEFAULT_BORDER_COLOR = Color.from_ok_hsl(0, 0, 0.5, 1)

static func _static_init() -> void:
    # build the textures for an empty button
    var base = [
        Template.COLOR, TEMPLATE_CHANNEL_BG, DEFAULT_BG_COLOR,
        Template.COLOR, TEMPLATE_CHANNEL_BG_STRIPE, DEFAULT_BG_STRIPE_COLOR,
    ]

    var normal := build_image(base + [
        Template.COLOR, TEMPLATE_CHANNEL_BORDER, DEFAULT_BORDER_COLOR,
    ])
    empty_textures["normal"] = ImageTexture.create_from_image(normal)

    var bitmap := BitMap.new()
    bitmap.create_from_image_alpha(normal)
    empty_textures["click_mask"] = bitmap

    empty_textures["hover"] = ImageTexture.create_from_image(
        build_image(base + [
            Template.COLOR, TEMPLATE_CHANNEL_BORDER, Color.GOLD,
        ]))
    empty_textures["pressed"] = ImageTexture.create_from_image(
        build_image(base + [
            Template.COLOR, TEMPLATE_CHANNEL_BORDER, Color.WHITE,
        ]))

# build a hex cell Image, coloring various parts of the template image as
# requested
static func build_image(steps: Array) -> Image:
    var size = cell_template.get_size()

    var image := Image.create(size.x, size.y, false, Image.FORMAT_RGBA8)
    image.fill(Color.from_hsv(0,0,0,0))
    image.set_pixel(size.x -1, size.y -1, Color.CORAL)
    image.resize(size.x, size.y)

    while not steps.is_empty():
        var step: Template = steps.pop_front()
        match step:
            Template.COLOR:
                var channel: int = steps.pop_front()
                var color: Color = steps.pop_front()
                for x in range(size.x):
                    for y in range(size.y):
                        var adj = cell_template.get_pixel(x, y)[channel]
                        if adj > 0.5:
                            image.set_pixel(x, y, color * adj)
            Template.ICON:
                var icon = icon_templates[steps.pop_front()]
                for x in range(size.x):
                    for y in range(size.y):
                        var color = icon.get_pixel(x, y)
                        if color.a > 0.5:
                            image.set_pixel(x, y, color)

                pass
            _:
                push_error("build_image(): unsupported step ", step)

    # resize the image down to the size we need
    # we're doing all this work at a higher resolution then scaling to reduce
    # visual artifacts from manually setting the pixels that have multiple
    # channels set.
    size = IMAGE_SIZE * EditorInterface.get_editor_scale()
    image.resize(size.x, size.y) # , Image.INTERPOLATE_LANCZOS)
    return image
    return image

func update_textures() -> void:
    # If nothing is customized, use the default templates
    if !color && !border_color && icon == Icon.None:
        texture_normal = empty_textures["normal"]
        texture_hover = empty_textures["hover"]
        texture_pressed = empty_textures["pressed"]
        texture_click_mask = empty_textures["click_mask"]
        return

    print("rebuilding textures")
    var iteration_begin_usec = Time.get_ticks_usec()
    var base = []
    if color == null:
        base.append_array([
            Template.COLOR, TEMPLATE_CHANNEL_BG, DEFAULT_BG_COLOR,
            Template.COLOR, TEMPLATE_CHANNEL_BG_STRIPE, DEFAULT_BG_STRIPE_COLOR,
        ])
    else:
        base.append_array([ Template.COLOR, TEMPLATE_CHANNEL_BG, color ])

    if icon != Icon.None:
        base.append_array([ Template.ICON, icon ])

    var normal := build_image(base + [
        Template.COLOR, TEMPLATE_CHANNEL_BORDER,
            border_color if border_color else DEFAULT_BORDER_COLOR
    ])
    texture_normal = ImageTexture.create_from_image(normal)

    var bitmap := BitMap.new()
    bitmap.create_from_image_alpha(normal)
    texture_click_mask = bitmap

    texture_hover = ImageTexture.create_from_image(
        build_image(base + [
            Template.COLOR, TEMPLATE_CHANNEL_BORDER, Color.GOLD
        ]))
    texture_pressed = ImageTexture.create_from_image(
        build_image(base + [
            Template.COLOR, TEMPLATE_CHANNEL_BORDER, Color.WHITE
        ]))
    var duration = Time.get_ticks_usec() - iteration_begin_usec
    print("texture rebuild took ", duration, "us")

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    update_textures()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    pass

func _get_minimum_size() -> Vector2:
    return IMAGE_SIZE * EditorInterface.get_editor_scale()

