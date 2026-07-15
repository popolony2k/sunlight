# collision sample

Builds on [sprite](/samples/sprite/docs/README.md) by making Sunny keyboard-controllable and giving it something to bump into — a static house obstacle. This is the smallest example of `sunlight`'s collision API: `CollisionManager`, `AddColliderToColliderRule`, and the `ICollisionListener` callback.

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

Unlike the `tilemaprenderer`/`sprite` samples, these keys move Sunny directly rather than scrolling the camera — there's no inversion here, since the character is visible on screen (see `feedback_camera_keybindings` in this project's notes if you're comparing against the other two samples' camera controls).

Walk Sunny to the right into the house — movement stops at its edge instead of walking through it.

## What it shows

- `World::LoadSprites()` ([world.cpp](/samples/collision/world.cpp)) loads Sunny (animated, as in the `sprite` sample) and a static house tile (`resources/map/images/house.png`, pinned to a single non-animated frame via `TextureCanvas::SetTileSize`) as two separate `Sprite`s, added to the renderer on two different layer ids.
- Every `Sprite` already owns a `Collider` internally, and `TileMapRenderer::AddSprite()` automatically registers it with the renderer's `CollisionManager` on that same layer id — no manual collider wiring needed.
- `GetCollisionManager().AddColliderToColliderRule(sunnyLayerId, houseLayerId)` tells the manager to check those two layers against each other every frame.
- `World` implements `ICollisionListener::OnCollision()`. When it fires, the sample undoes the movement step that caused the overlap, giving the house solid, wall-like collision — a real game would trigger gameplay logic here instead (damage, pickups, opening a door, ...).

## Resources

- `resources/map/` — the Tiled `.tmx` map and its tilesets, shared with the other samples.
- `resources/sprites/sunny_idle_down.png` — the sprite sheet used for the animated, player-controlled character.
