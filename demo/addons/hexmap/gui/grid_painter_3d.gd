@tool
extends SubViewportContainer

@export var cells := [
        {"offset": HexMapCellId.at(0,0,0), "state": "type", "type": 0},
    ] :
    set(value):
        cells = value
        populate_cells()
@export var cell_types := [] :
    set(value):
        cell_types = value
        populate_cells()
@export var tile_mesh : Mesh :
    set(value):
        tile_mesh = value
        populate_cells()
@export var selected_type := -1


# camera controls
var y_rot := 0.0
var x_rot := deg_to_rad(45)
var distance := 10.0
var edit_plane := Plane(Vector3.UP)
var cursor_cell := HexMapCellId.new()

# hex map settings
var hex_space := HexMapSpace.new()

func set_hex_space(value: HexMapSpace) -> void:
    hex_space = value

func reset() -> void:
    cells.clear()
    cell_types.clear()
    selected_type = -1

func _replace_cell(offset: HexMapCellId, state: String, type):
    for cell in cells:
        if cell.offset.equals(offset):
            cell.state = state
            cell.type = type
            return

func populate_cells() -> void:
    if not is_node_ready():
        await ready

    for child in %CellMeshes.get_children():
        %CellMeshes.remove_child(child)
        child.queue_free()

    for cell in cells:
        var instance := MeshInstance3D.new()
        instance.mesh = build_cell_mesh(cell)
        instance.position = hex_space.get_cell_center(cell.offset)
        %CellMeshes.add_child(instance)
    pass

func build_cell_mesh(desc: Dictionary) -> Mesh:
    var mesh := CylinderMesh.new()
    mesh.top_radius = hex_space.cell_radius * 0.95
    mesh.bottom_radius = hex_space.cell_radius * 0.95
    mesh.rings = 1
    mesh.radial_segments = 6
    mesh.height = 0.2 * hex_space.cell_height
    match desc.state:
        "empty":
            pass
        "not_empty":
            pass
        "type":
            var mat := StandardMaterial3D.new()
            for type in cell_types:
                if type.value == desc.type:
                    mat.albedo_color = type.color
                    break
            mesh.material = mat
            pass
        "not type":
            pass
        "disabled":
            pass
        _:
            push_error("unsupported cell state ", desc.state)

    return mesh

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    _orbit_camera(Vector2())
    mouse_entered.connect(func(): grab_focus())
    pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    pass

func _gui_input(event: InputEvent) -> void:
    var cell_changed = false
    # update cursor cell
    if event is InputEventMouse:
        cell_changed = _update_cursor_cell(event.position)

    # camera controls
    if event is InputEventMouseMotion \
            and event.button_mask & MOUSE_BUTTON_MASK_MIDDLE:
        _orbit_camera(event.relative)
    elif event is InputEventMouseButton \
            and event.button_index == MOUSE_BUTTON_WHEEL_UP \
            and event.is_pressed():
        var zoom_factor = 1 + (1.08 - 1) * event.factor
        _scale_camera_distance(1.0/zoom_factor)
    elif event is InputEventMouseButton \
            and event.button_index == MOUSE_BUTTON_WHEEL_DOWN \
            and event.is_pressed():
        var zoom_factor = 1 + (1.08 - 1) * event.factor
        _scale_camera_distance(zoom_factor)

    # painting
    if event is InputEventMouseButton \
            and event.button_index == MOUSE_BUTTON_LEFT \
            and event.is_pressed() \
            and selected_type != -1:
        _replace_cell(cursor_cell, "type", selected_type)
        populate_cells()
    # erasing
    elif event is InputEventMouseButton \
            and event.button_index == MOUSE_BUTTON_RIGHT \
            and event.is_pressed():
        _replace_cell(cursor_cell, "disabled", -1)
        populate_cells()

func _input(event: InputEvent) -> void:
    if event is InputEventKey:
        print("focused ", has_focus(), ", key ", event)
        accept_event()

func _orbit_camera(relative: Vector2) -> void:
    # adapted from Node3DEditorViewport::_nav_orbit()

    var editor_settings = EditorInterface.get_editor_settings()
    var degrees_per_pixel = editor_settings.get_setting("editors/3d/navigation_feel/orbit_sensitivity");
    var radians_per_pixel = deg_to_rad(degrees_per_pixel);
    var invert_y_axis = editor_settings.get_setting("editors/3d/navigation/invert_y_axis")
    var invert_x_axis = editor_settings.get_setting("editors/3d/navigation/invert_x_axis")

    if invert_y_axis:
        x_rot -= relative.y * radians_per_pixel
    else:
        x_rot += relative.y * radians_per_pixel
    x_rot = clamp(x_rot, -1.57, 1.57)

    if invert_x_axis:
        y_rot -= relative.x * radians_per_pixel
    else:
        y_rot += relative.x * radians_per_pixel

    _update_camera_transform()

func _scale_camera_distance(scale: float) -> void:
    # XXX set distance limits based off HexSpace cell size
    distance = clamp(distance * scale, 2, 30)
    _update_camera_transform()

func _update_camera_transform():
    # from Node3DEditorViewport::_update_camera
    var camera_transform := Transform3D()
    camera_transform = camera_transform.rotated(Vector3(1, 0, 0), -x_rot)
    camera_transform = camera_transform.rotated(Vector3.UP, -y_rot)
    camera_transform = camera_transform \
        .translated_local(Vector3(0, 0, distance))
    %Camera3D.transform = camera_transform

    var half_fov = deg_to_rad(35)/2
    var height = 2 * distance * tan(half_fov)
    %Camera3D.size = height

func _update_cursor_cell(pos: Vector2) -> Variant:
    var origin = %Camera3D.project_ray_origin(pos)
    var normal = %Camera3D.project_ray_normal(pos)
    var intersect = edit_plane.intersects_ray(origin, normal)
    if not intersect:
        return false
    var cell_id = hex_space.get_cell_id(intersect)
    if cell_id.equals(cursor_cell):
        return false
    cursor_cell = cell_id
    return true
