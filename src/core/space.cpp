#include "space.h"

void HexMapSpace::set_transform(Transform3D value) { transform = value; }

void HexMapSpace::set_cell_scale(Vector3 value) { cell_scale = value; }
void HexMapSpace::set_cell_height(real_t value) { cell_scale.y = value; }
void HexMapSpace::set_cell_radius(real_t value) {
    cell_scale.x = cell_scale.z = value;
}

void HexMapSpace::set_mesh_offset(Vector3 value) { mesh_offset = value; }
