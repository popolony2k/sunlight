# hexagonal sample

Loads a hexagonal Tiled map (`orientation="hexagonal"`, `hexsidelength="20"`, `staggeraxis="y"`, `staggerindex="odd"`) and walks Sunny across it one hex at a time, in all six directions. It follows the [isometric](/samples/isometric/docs/README.md) and [staggered](/samples/staggered/docs/README.md) samples.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `hexagonal_test` executable under `build/samples/hexagonal/`, along with the shared libraries copied next to it.

## Running

`hexagonal_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/hexagonal/hexagonal_test samples/hexagonal/
```

## Controls

| Keys | Action |
| --- | --- |
| `Q` / `E` | One hex up-left (NW) / up-right (NE) |
| `A` / `D` | One hex left (W) / right (E) |
| `Z` / `C` | One hex down-left (SW) / down-right (SE) |
| `+` / `-` (keyboard or keypad) | Zoom in / out |
| `=` (keyboard or keypad) or `Home` | Reset the zoom |
| `Esc` | Quit |

The six keys sit around `S` on a QWERTY keyboard, in the same directions as the hex's neighbours.

## What to look for

- The map is an 8 x 6 grid of 60 x 50 hexes. Rows are `(tile height + hex side length) / 2` = 35 pixels apart, so neighbouring rows interlock, and the odd rows are shifted right by half a hex.
- The left and right neighbours are in the same row. The four diagonal neighbours depend on whether the current row is even or odd, as on the staggered map.
- A step into an obstacle hex, or off the map, does nothing (a `GetTile` check on the "obstacles" layer). Sunny glides between hexes, and the camera scrolls when she reaches the viewport's edge.

## In the code

- `World::TryMoveHex()` ([world.cpp](/samples/hexagonal/world.cpp)) turns a direction into the target hex, checks it, and starts the glide.
- `World::CellTopLeft()` converts a hex to its map-pixel position with the hexagonal formula.

## Assets

- `resources/map/hexagonal.tmx`, `tileset_hexagonal.tsx` and `tileset_hex_obstacle.tsx`, with their images in `resources/map/images/`.
- `resources/sprites/sunny_idle_down.png`: Sunny.
