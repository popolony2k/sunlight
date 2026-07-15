# sprite sample

This sample builds on [tilemaprenderer](/samples/tilemaprenderer/docs/README.md) by adding an animated `Sprite` ("Sunny") on top of the loaded map — it's the smallest example of `sunlight`'s sprite/animation API: loading a texture into a `TextureCanvas`, wrapping it in a `Sprite` texture sequence, and registering it with the renderer on a specific map layer.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `sprite_test` executable under `build/samples/sprite/`, along with the raylib/tmx/LibXml2/sunlight shared libraries copied next to it (required at runtime).

## Running

`sprite_test` resolves its map/sprite resources relative to a base path passed as `argv[1]` — point it at this sample's own directory:

```shell
./build/samples/sprite/sprite_test samples/sprite/
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

The current FPS is drawn in the corner of the window.

## What it shows

- `World::LoadSprites()` ([world.cpp](/samples/sprite/world.cpp)) loads `resources/sprites/sunny_idle_down.png` into a `TextureCanvas`, splits it into 32×32 tiles, and sets `TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR` so it cycles through the frames on its own.
- The canvas is added as sequence `0` of a `Sprite`, which is then registered on the renderer's layer `4` via `TileMapRenderer::AddSprite()` — sprites are drawn per-layer, interleaved with the tilemap's own layers.
- The rest of `World` mirrors the [tilemaprenderer sample](/samples/tilemaprenderer/docs/README.md): same camera/zoom controls, same `resources/map/test.tmx` map, same viewport setup.

## Resources

- `resources/map/` — the Tiled `.tmx` map and its tilesets (shared layout with the `tilemaprenderer` sample).
- `resources/sprites/sunny_idle_down.png` — the sprite sheet used for the animated character.
