**Godot 4 GDExtension for creating 3D hexagonal maps**

This is a **work-in-progress** adaptation of the Godot GridMap Node to support hexagonal cells.

At this time the core functionality works, but a lot of polish is still needed.

**The API for `HexMap` is not yet stable**

<img width="1547" alt="Screenshot 2024-09-20 at 9 47 40 PM" src="https://github.com/user-attachments/assets/5885af74-9fc5-44cc-a088-8e6c252577d2">

## Features
* Hexagonal shaped cells in `HexMap` (https://github.com/godotengine/godot/pull/85890)
* `HexMap` cell id helper class for navigating hexagonal space
    * See `HexMapCellIdRef` (name will change)
* Signal added for when a cell changes (https://github.com/godotengine/godot/issues/11855)

## Important Changes
* Hot keys have changed; check the menu in the editor

## Absent functionality from GirdMap
* Currently the `HexMap` node does not support navigation mesh generation
    * The API for `godot-cpp` doesn't provide the same interface; needs alternative
