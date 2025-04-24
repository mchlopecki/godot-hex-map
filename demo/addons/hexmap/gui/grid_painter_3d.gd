@tool
@static_unload
extends SubViewportContainer


signal cell_clicked(cell_id: HexMapCellId, mouse_button: int)

@export var cell_types := [] :
    set(value):
        cell_type_color.clear()
        for type in cell_types:
            cell_type_color[type.value] = type.color
@export var tile_mesh : Mesh :
    set(value):
        tile_mesh = value
@export var selected_type := -1

# map cell_type to color
var cell_type_color := {}
var cell_instances := {}

class HexCell:
    extends Node3D
    const Math_SQRT3_2 = 0.8660254037844386
    const BOTTOM_OFFSET = Vector3(0, -0.5, 0)
    
    @export var state : Array :
        set(value):
            state = value
            _on_state_changed()

    var floor_mesh_instance: MeshInstance3D
    var cell_mesh_instance: MeshInstance3D
    var state_node: Node3D

    static var tile_mesh: Mesh = preload("../models/autotiled_rule_tile.obj")
    static var tile_not_mesh: Mesh = preload("../models/autotiled_rule_tile_not.obj")
    static var tile_any_mesh: Mesh = preload("../models/autotiled_rule_tile_any.obj")

    # storing all statics in a Dictionary because hot-reload doesn't work
    # properly for static variables.
    # see https://github.com/godotengine/godot/issues/105667
    static var shared = {}

    static func _static_init() -> void:
        # Grid material 
        var mat := StandardMaterial3D.new()
        mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
        mat.disable_fog = true
        mat.disable_receive_shadows = true
        mat.albedo_color = Color.GOLD
        shared["grid_mat"] = mat

        # grid mesh
        var st = SurfaceTool.new()
        st.begin(Mesh.PRIMITIVE_LINE_STRIP)
        st.set_material(mat)
        st.add_vertex(Vector3(0.0, 0, -1.0))
        st.add_vertex(Vector3(-Math_SQRT3_2, 0, -0.5))
        st.add_vertex(Vector3(-Math_SQRT3_2, 0, 0.5))
        st.add_vertex(Vector3(0.0, 0, 1.0))
        st.add_vertex(Vector3(Math_SQRT3_2, 0, 0.5))
        st.add_vertex(Vector3(Math_SQRT3_2, 0, -0.5))
        st.add_vertex(Vector3(0.0, 0, -1.0))
        shared["grid_mesh"] = st.commit()

    # just keep track of our children
    func _ready() -> void:
        floor_mesh_instance = MeshInstance3D.new()
        floor_mesh_instance.mesh = shared.grid_mesh
        print("add floor mesh child")
        add_child(floor_mesh_instance)

    # update the meshes for this node based on state
    func _on_state_changed():
        if state_node != null:
            remove_child(state_node)
            state_node.queue_free()
            state_node = null
        match state[0]:
            "disabled":
                pass
            "empty":
                state_node = MeshInstance3D.new()
                state_node.mesh = tile_not_mesh
                add_child(state_node)
                pass
            "not_empty":
                state_node = MeshInstance3D.new()
                state_node.mesh = tile_any_mesh
                add_child(state_node)
                pass
            "type":
                var mat := StandardMaterial3D.new()
                mat.albedo_color = state[1]
                state_node = MeshInstance3D.new()
                state_node.mesh = tile_mesh
                state_node.material_override = mat
                add_child(state_node)
            "not_type":
                var mat := StandardMaterial3D.new()
                mat.albedo_color = state[1]
                state_node = MeshInstance3D.new()
                state_node.mesh = tile_mesh
                state_node.material_override = mat
                var decor = MeshInstance3D.new()
                decor.mesh = tile_not_mesh
                state_node.add_child(decor)
                add_child(state_node)

                pass
            _:
                push_error("unsupported cell state ", state)



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

func set_cell(cell_id: HexMapCellId, state: Array) -> void:
    var cell = cell_instances.get_or_add(cell_id.as_int(), HexCell.new())
    cell.state = state
    cell.position = hex_space.get_cell_center(cell_id)
    cell.scale = hex_space.get_cell_scale()

    if not cell.is_inside_tree():
        %CellMeshes.add_child(cell)

func reset() -> void:
    # XXX clear child cells
    cell_types.clear()
    selected_type = -1


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

    # painting/erasing
    if event is InputEventMouseButton and event.is_pressed():
        cell_clicked.emit(cursor_cell, event.button_index)

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
