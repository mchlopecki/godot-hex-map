#include "space.h"

void HexSpace::set_transform(Transform3D value) { transform = value; }

void HexSpace::set_cell_scale(Vector3 value) { cell_scale = value; }
void HexSpace::set_cell_height(real_t value) { cell_scale.y = value; }
void HexSpace::set_cell_radius(real_t value) {
    cell_scale.x = cell_scale.z = value;
}

void HexSpace::set_mesh_offset(Vector3 value) { mesh_offset = value; }
