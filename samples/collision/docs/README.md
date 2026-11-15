# collision sample

Builds on [sprite](/samples/sprite/docs/README.md) by making Sunny keyboard-controllable and giving it things to bump into: a static obstacle sprite, and a box of wall tiles. It shows `sunlight`'s collision API: `CollisionManager`, `AddColliderToColliderRule` (sprite against sprite), `AddColliderToTileRule` (sprite against map tiles that have a Tiled collision shape), and the `ICollisionListener` callbacks.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `collision_test` executable under `build/samples/collision/`, along with the raylib/tmx/LibXml2/sunlight shared libraries copied next to it (required at runtime).

## Running

`collision_test` resolves its map/sprite resources relative to a base path passed as `argv[1]` — point it at this sample's own directory:

```shell
./build/samples/collision/collision_test samples/collision/
```

## Controls

| Keys | Action |
| --- | --- |
| `W` / `Up Arrow` | Move Sunny up |
| `S` / `Down Arrow` | Move Sunny down |
| `A` / `Left Arrow` | Move Sunny left |
| `D` / `Right Arrow` | Move Sunny right |
| `Page Up` | Zoom out |
| `Page Down` | Zoom in |
| `Home` | Reset zoom to the default level |
| `+` / `-` / `=` (keyboard or keypad) | Zoom in / out / reset the zoom |

Unlike the `orthogonal`/`sprite` samples, these keys move Sunny directly rather than scrolling the camera — there's no inversion here, since the character is visible on screen (see `feedback_camera_keybindings` in this project's notes if you're comparing against the other two samples' camera controls).

Walk Sunny to the right into the obstacle: movement stops at its edge instead of walking through it.

Walk Sunny into the brick walls around her from every side. She stops at each wall's edge, from the left, the right, above and below. The gap on the right of the box lets her out. Zoom in and out, and the walls still stop her exactly at their edges.

## What it shows

- `World::LoadSprites()` ([world.cpp](/samples/collision/world.cpp)) loads Sunny (animated, as in the `sprite` sample) and a static obstacle (`resources/map/images/monke_variants.png`, tile 0 of its 4x4 grid — each cell there is a self-contained square icon, unlike e.g. the house tileset's diagonal roof pieces, which don't work as a single static tile) as two separate `Sprite`s, added to the renderer on two different layer ids.
- Every `Sprite` already owns a `Collider` internally, and `TileMapRenderer::AddSprite()` automatically registers it with the renderer's `CollisionManager` on that same layer id — no manual collider wiring needed.
- `GetCollisionManager().AddColliderToColliderRule(sunnyLayerId, obstacleLayerId)` tells the manager to check those two layers against each other every frame.
- `World` implements `ICollisionListener::OnCollision()`. When it fires, the sample undoes the movement step that caused the overlap, giving the obstacle solid, wall-like collision — a real game would trigger gameplay logic here instead (damage, pickups, opening a door, ...).
- **The walls** are a tile layer, `walls` (id 9), drawn with `tileset_wall.tsx`. Its one tile has a collision rectangle over the whole tile, drawn in Tiled's Tile Collision Editor. `AddColliderToTileRule(sunnyLayerId, 9)` checks Sunny against every tile of that layer.
- **The tile callback:** `OnCollision(Collider*, stTile*)` fires once for each wall tile Sunny touches, with the tile's rectangle in map space. Touching two walls at once, in a corner, gives two calls, so `UndoLastStep()` undoes the frame's movement only once. That movement is the sum of every step taken that frame (`Step()`): a diagonal (two keys held) moves Sunny on both axes in one frame, and both are undone, so she stops at a wall rather than sliding along it.
- **Map space:** tile collisions are worked out in map space. The camera (the map is centre-aligned) and the zoom don't move where Sunny stops. Two rectangles that only share an edge don't touch, so Sunny can rest flush against a wall.

## Resources

- `resources/map/` — the Tiled `.tmx` map and its tilesets, shared with the other samples, plus the `walls` layer and `tileset_wall.tsx` (with `images/wall.png`, a brick tile), which are this sample's own.
- `resources/sprites/sunny_idle_down.png` — the sprite sheet used for the animated, player-controlled character.
