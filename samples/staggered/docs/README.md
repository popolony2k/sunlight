# staggered sample

Loads a staggered Tiled map (`orientation="staggered"`, `staggeraxis="y"`, `staggerindex="odd"`) and walks Sunny across it one cell at a time. It is the [isometric sample](/samples/isometric/docs/README.md) on a staggered grid: every other row is shifted by half a tile.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `staggered_test` executable under `build/samples/staggered/`, along with the shared libraries copied next to it.

## Running

`staggered_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/staggered/staggered_test samples/staggered/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | One cell up-right (NE) |
| `S` / `Down Arrow` | One cell down-left (SW) |
| `A` / `Left Arrow` | One cell up-left (NW) |
| `D` / `Right Arrow` | One cell down-right (SE) |
| `+` / `-` (keyboard or keypad) | Zoom in / out |
| `=` (keyboard or keypad) or `Home` | Reset the zoom |
| `Esc` | Quit |

## What to look for

- The map is an 8 x 6 grid of 64 x 48 diamond tiles. The odd rows are shifted right by half a tile, so the map is a rectangle instead of the isometric diamond.
- A diagonal neighbour is in a different column depending on whether the current row is even or odd. Each key still reaches the right cell from either kind of row.
- A step into an obstacle cell, or off the map, does nothing (a `GetTile` check on the "obstacles" layer). Sunny glides between cells, and the camera scrolls when she reaches the viewport's edge.

## In the code

- `World::TryMoveStep()` ([world.cpp](/samples/staggered/world.cpp)) turns a direction into the target cell for the current row's parity, checks it, and starts the glide.
- `World::CellTopLeft()` converts a cell to its map-pixel position with the staggered formula.

## Assets

- `resources/map/staggered.tmx`, `tileset_staggered.tsx` and `tileset_obstacle.tsx`, with their images in `resources/map/images/`.
- `resources/sprites/sunny_idle_down.png`: Sunny.
