# surface sample

Uses `TileMapRenderer` purely as a drawing surface: `LoadMap` is never called. Everything on screen is drawn in code: shapes through `IShapeSurface`, an animated sprite registered with `AddSprite`, and a HUD through `IDrawSurface`.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `surface_test` executable under `build/samples/surface/`, along with the shared libraries copied next to it.

## Running

`surface_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/surface/surface_test samples/surface/
```

## Controls

| Keys | Action |
| --- | --- |
| Arrow keys | Move Sunny |
| `Esc` | Quit |

## What to look for

- **Shapes, inside the viewport:** a thick cyan square turning about its centre, a thick orange ellipse ring turning more slowly, a thick green rectangle, a magenta sine wave travelling along a thick polyline, and a row of nine yellow points, each larger than the last. Positions, thickness and point size are in map units, so the renderer converts them like a Tiled shape object, and the viewport clips them.
- **Sunny:** an animated sprite with no map loaded. Its layer id (1) is only a grouping key.
- **The HUD, outside the viewport:** a line of text and a progress bar that fills and starts again. These are in raw screen pixels, so the viewport doesn't clip them.

## In the code

- `World::OnUpdate()` ([world.cpp](/samples/surface/world.cpp)) draws the shapes and the HUD every frame. `Run()` calls it inside the frame's render target, after the sprites, so this is the place a game with no map draws its own content.
- `World::DrawShapes()` uses `IShapeSurface`: `DrawPolygon`, `DrawEllipse`, `DrawRectangle`, `DrawPolyline` and `DrawPoint`, each with a thickness and, for the square and the ring, a rotation.
- `World::DrawHud()` uses `IDrawSurface::DrawText` and `DrawFilledRectangle`.
- `World::LoadSprites()` registers Sunny with `AddSprite` on layer 1. With no map loaded, the renderer draws registered sprites in ascending layer-id order.

## Assets

- `resources/sprites/sunny_idle_down.png`: Sunny.
