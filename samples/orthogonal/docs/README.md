# orthogonal sample

The simplest `sunlight` sample: loads an orthogonal Tiled `.tmx` map and drives `TileMapRenderer`'s game loop, camera scrolling, and zoom — no sprites, no collisions, just the map renderer on its own. See the [sprite sample](/samples/sprite/docs/README.md) for the same setup with an animated character added on top.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `orthogonal_test` executable under `build/samples/orthogonal/`, along with the raylib/tmx/LibXml2/sunlight shared libraries copied next to it (required at runtime).

## Running

`orthogonal_test` resolves its map resources relative to a base path passed as `argv[1]` — point it at this sample's own directory:

```shell
./build/samples/orthogonal/orthogonal_test samples/orthogonal/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | Scroll the view up |
| `S` / `Down Arrow` | Scroll the view down |
| `A` / `Left Arrow` | Scroll the view left |
| `D` / `Right Arrow` | Scroll the view right |
| `Page Up` | Zoom out |
| `Page Down` | Zoom in |
| `Home` | Reset zoom to the default level |
| `+` / `-` / `=` (keyboard or keypad) | Zoom in / out / reset the zoom |

The current FPS is drawn in the corner of the window.

## What it shows

- `World::Run()` ([world.cpp](/samples/orthogonal/world.cpp)) constructs a `TileMapRenderer`, sets up an 890×790 viewport at (10, 10) with a default zoom level, wires the controls above via `SetUserKeyEventHandler`, then loads `resources/map/test.tmx` centered in the window (`MAP_ALIGNMENT_CENTER`) before entering the render loop with `Run()`.
- This is the minimal setup needed to get a Tiled map on screen — everything else in the library (sprites, collisions, sound, scripting) builds on top of this same `TileMapRenderer` instance.

## Resources

- `resources/map/` — the Tiled `.tmx` map and its tilesets, shared with the `sprite` sample.
