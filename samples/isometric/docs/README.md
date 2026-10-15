# isometric sample

Loads an isometric Tiled map (`orientation="isometric"`) and walks Sunny across it one cell at a time. Cells holding an obstacle tile can't be entered, and the camera scrolls when Sunny reaches the viewport's edge.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `isometric_test` executable under `build/samples/isometric/`, along with the shared libraries copied next to it.

## Running

`isometric_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/isometric/isometric_test samples/isometric/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | One cell up-right (row - 1) |
| `S` / `Down Arrow` | One cell down-left (row + 1) |
| `A` / `Left Arrow` | One cell up-left (column - 1) |
| `D` / `Right Arrow` | One cell down-right (column + 1) |
| `+` / `-` (keyboard or keypad) | Zoom in / out |
| `=` (keyboard or keypad) or `Home` | Reset the zoom |
| `Esc` | Quit |

## What to look for

- The map is an 8 x 6 grid of 64 x 32 diamond tiles. Each key moves Sunny along one axis of the grid, which on screen is a diagonal.
- Sunny glides from cell to cell instead of jumping. While she moves, further key presses are ignored.
- A step into an obstacle cell, or off the map, does nothing. The check reads the "obstacles" layer with `GetTile`. A collider-to-tile rule on the same layer is also registered, to show that collisions work on an isometric map.
- When a step would take Sunny past the viewport's edge, the camera scrolls instead, glides in step with her, and stops at the map's edge.

## In the code

- `World::Run()` ([world.cpp](/samples/isometric/world.cpp)) loads `resources/map/isometric.tmx` with `MAP_ALIGNMENT_TOP_LEFT` and registers the controls.
- `World::TryMoveStep()` checks the target cell, starts the glide, and works out whether the camera has to scroll.
- `World::CellTopLeft()` converts a cell to its map-pixel position with the isometric formula.

## Assets

- `resources/map/isometric.tmx`, `tileset_isometric.tsx` and `tileset_obstacle.tsx`, with their images in `resources/map/images/`.
- `resources/sprites/sunny_idle_down.png`: Sunny.
