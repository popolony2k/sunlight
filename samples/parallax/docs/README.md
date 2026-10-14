# parallax sample

Shows Tiled's per-layer parallax: layers scroll at different speeds as the camera moves, giving a sense of depth. Sunny stays at the centre of the screen; walking moves the camera, and the map wraps around so the walk never ends.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `parallax_test` executable under `build/samples/parallax/`, along with the shared libraries copied next to it.

## Running

`parallax_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/parallax/parallax_test samples/parallax/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | Walk up |
| `S` / `Down Arrow` | Walk down |
| `A` / `Left Arrow` | Walk left |
| `D` / `Right Arrow` | Walk right |
| `Page Up` / `Page Down` | Zoom out / in |
| `Home` | Reset the zoom |
| `Esc` | Quit |

Holding a key keeps walking (4 map pixels per frame).

## What to look for

The layers of `resources/map/parallax.tmx`, back to front, with their Tiled `parallaxx`/`parallaxy` factors:

| Layer | Factor | Moves with the camera at |
| --- | --- | --- |
| `terrain`, `buildings` | 1 | full speed, like a normal layer |
| `sky` (opacity 0.85), `birds` | 0.25 | a quarter of the speed: far away |
| `clouds` (opacity 0.95) | 0.5 | half the speed: in between |

- Walk in any direction: the ground and houses pass at full speed, the clouds slower, and the sky and birds slowest.
- Sunny is on the `clouds` layer, the last one, so she is drawn over everything. She is a screen-space sprite, so no layer's factor moves her.
- The map wraps: the four marker objects `wrap_left`, `wrap_right`, `wrap_top` and `wrap_bottom` mark a rectangle, and crossing one edge carries Sunny to the opposite one. sunlight has no built-in wrap; the sample reads the markers with `GetObjectByName` and wraps her position itself.

## In the code

- The factors come from the map file; nothing in the code sets them. A game can also set them at run time with `TileMapRenderer::SetLayerParallax(layer, fx, fy)`.
- `World::LoadWrapMarkers()` ([world.cpp](/samples/parallax/world.cpp)) reads the markers, `World::WrapWorldPosition()` wraps Sunny's map position, and `World::UpdateCameraOnSunny()` centres the camera on it after every step and zoom change.
- `World::RecenterSunnyOnScreen()` keeps the sprite at the viewport's centre at any zoom: a screen-space sprite's position is multiplied by the zoom, so it stores the centre divided by the zoom.

## Assets

- `resources/map/parallax.tmx` and its eight tilesets (`tileset_*.tsx`), with their images in `resources/map/images/`.
- `resources/sprites/sunny_idle_down.png`: Sunny.
