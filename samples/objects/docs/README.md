# objects sample

Shows Tiled tile objects (`OT_TILE`): a tile placed as an object rather than in a tile layer. One row of the map is one case, and each row has a numbered label beside it that names the case.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `objects_test` executable under `build/samples/objects/`, along with the shared libraries copied next to it.

## Running

`objects_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/objects/objects_test samples/objects/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | Scroll the view up |
| `S` / `Down Arrow` | Scroll the view down |
| `A` / `Left Arrow` | Scroll the view left |
| `D` / `Right Arrow` | Scroll the view right |
| `Esc` | Quit |

## What to look for

The green rectangle is the viewport's frame in the map. Anything drawn outside it must not be visible.

- **1. tile object at its own size (32 px)**: the tile is drawn at the size of its tile.
- **2. resized: stretched to its 64 px box**: the object is 64 px, so the tile is stretched to fill that box, as Tiled draws it.
- **3. cut by the left viewport edge**: the object starts left of the frame, so its left part is hidden.
- **4. animated: the tile cycles its frames**: tile 3 of the house tileset cycles through four frames, so the object changes picture over time.
- **5. flip bits ignored: drawn unflipped**: the gid has the horizontal-flip bit set. The flip is not drawn yet, so the tile looks the same as case 1.
- **6. cut by the bottom viewport edge**: the object starts inside the frame and is cut by its bottom edge.

Tiled places a tile object by its bottom-left corner, so each tile's top-left is above the object's `y` by its height.

## Assets

- `resources/map/tileset_house.tsx` and `resources/map/images/house.png`: the house tileset from the [tilemaprenderer sample](../../tilemaprenderer/docs/README.md).
- `../shared/fonts/caravellius8x8.fnt` and `.png`: Caravellius 8x8, designed by PopolonY2k and Leidson Campos A. Ferreira. Dual-licensed under the zlib License or the SIL Open Font License 1.1 (`../shared/fonts/OFL-caravellius8x8.txt`).
