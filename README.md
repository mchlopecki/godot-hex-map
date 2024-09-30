**Godot 4 GDExtension for creating 3D hexagonal maps**

This GDExtension is a rewrite of the Godot GridMap module to support hexagon
cells, and improve its maintainability.

At this time, the `HexMap` node has feature parity with `GridMap` barring the
explicit removal of per-tile navigation meshes.

**The GDScript API for `HexMap` is not yet stable**

<img width="1547" alt="Screenshot 2024-09-20 at 9 47 40 PM" src="https://github.com/user-attachments/assets/5885af74-9fc5-44cc-a088-8e6c252577d2">

## Features
* Hexagonal shaped cells in `HexMap` (https://github.com/godotengine/godot/pull/85890)
* Supports `LightmapGI` for lightmap baking
* Supports NavigationMesh baking
    * Specific tiles in MeshLibrary can be excluded from baking (see demo)
* GDScript `HexMapCellId` cell id helper class for navigating hexagonal space
* HexMap `cells_changed` signal that inculdes cell ids for modified cells (https://github.com/godotengine/godot/issues/11855)
* Editor improvements
    * Variable selection axis (y-axis editing, camera rotation determines selection snapping)
    * Current tile hiding; allows painting tiles to show when smaller than existing tile

## Important Changes
* Hot keys have changed; check the menu in the editor
* Support for baked per-tile NavigationMesh removed

## Fixes from GridMap
* Cell selection works when HexMap is Transformed (https://github.com/godotengine/godot/issues/92655)

## TODOs
* [X] Add lightmap baking support
* [X] Mark MeshLibrary items as navigable/non-navigable
    * this can be used in NavMesh baking to include/exclude tile meshes
    * Either through navmesh presence in MeshLibrary, or through HexMap data
* [ ] GUI rewrite as PackedScene
    * Speed up GUI redesign
* [ ] Switch to bottom panel GUI
* [X] Preview placed tile in editor; existing cells can hide the cursor model
* [ ] Investigate copying custom data into baked mesh: https://github.com/godotengine/godot/issues/89254
    * don't know how to create a mesh with CUSTOM0, and where it is stored in the mesh/surface
* [ ] switch erasing to be a tool instead of right-click to allow free look
* [ ] clean up input handling in editor; consume all events during certain
        states to prevent odd behavior
* [ ] Editor selection modification support (grow up, down, etc)

## NavigationMesh
    * No support for per-tile NavigationMesh
    * NavigationRegion can bake using the HexMap
    * HexMap tiles can be exculded from NavigationMesh baking (set `navigation_bake_only_navmesh_tiles`)
