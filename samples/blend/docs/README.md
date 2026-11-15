# blend sample

Blend modes (`SunLight::Base::BlendMode`): how what is drawn next combines with what is already drawn. Runs over the orthogonal test map, or with no map loaded (`nomap`), since blend modes don't need a map. Three things use them:
- Sunny's glow, a sprite whose own mode the number keys choose (`Sprite::SetBlendMode`).
- A soft shadow sprite that always multiplies.
- Four panels the game draws itself, one per mode, between `IDrawSurface::BeginBlendMode` and `EndBlendMode`.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `blend_test` executable under `build/samples/blend/`, along with the shared libraries copied next to it.

## Running

`blend_test` resolves its resources relative to a base path passed as `argv[1]`. To run over the map:

```shell
./build/samples/blend/blend_test samples/blend/
```

To run with no map loaded:

```shell
./build/samples/blend/blend_test samples/blend/ nomap
```

## Controls

| Keys | Action |
| --- | --- |
| Arrow keys | Move Sunny, and its glow with it |
| `1` | Glow with no mode of its own: it draws by whatever mode is in effect (normal) |
| `2` | Glow in `BLEND_MODE_NORMAL` |
| `3` | Glow in `BLEND_MODE_ADD` (the default) |
| `4` | Glow in `BLEND_MODE_MULTIPLY` |
| `5` | Glow in `BLEND_MODE_SUBTRACT` |
| `Esc` | Quit |

## What to look for

- **The glow, drawn under Sunny:** in add, it brightens whatever is under it, map or sky, towards white at its centre. In multiply, it tints the scene warm. In subtract, it darkens towards blue. In normal (or with no mode), it is a plain warm halo over the scene.
- **The shadow (bottom left):** darkens what is under it, with soft edges. Its PNG's transparent pixels are white on purpose. raylib's own multiply would paint them white, but sunlight's multiply is exact, so no white box may show around the shadow.
- **The panels (right):** the same red, green and blue squares (alpha 200), one panel per mode, over the sky (the add panel over a dark night, where added colours mix instead of saturating to white):
  - **NORMAL:** the squares cover each other.
  - **ADD:** the colours add: red and green make yellow, and all three make white.
  - **MULTIPLY:** each square tints the sky, and overlaps get darker.
  - **SUBTRACT:** each square takes its own colour out of the sky.
- **The labels and the HUD:** drawn in normal mode after each panel's `EndBlendMode`, so they look the same in every panel.
- **Map and no map:** the same modes behave the same way over the map's layers and over a bare dusk-blue window.

## In the code

- **`World::DrawPanels()`** ([world.cpp](/samples/blend/world.cpp)): each panel's squares are drawn between `BeginBlendMode(mode)` and `EndBlendMode()`. `OnUpdate` calls it inside the frame, after the sprites.
- **`World::LoadSprites()`:** `m_Shadow.sprite.SetBlendMode(BLEND_MODE_MULTIPLY)` keeps the shadow in multiply on every frame. The sprite switches to its mode for its own draw only, then back to the mode in effect.
- **`World::SetGlowMode()`:** `SetBlendMode` or `ClearBlendMode` on the glow.
- **The font:** the HUD and the labels use `samples/shared/fonts/AnonymousPro-Bold.ttf` (loaded with `LoadFont`, then selected with `SetFont`), mounted at `/shared`.
- **No map:** with `nomap`, `LoadMap` is never called. The layer ids only order the sprites, and the window is cleared to a dusk blue (`SetWindowBackgroundColor`), darker than the sky-coloured panels.

## Assets

- `resources/map/`: the orthogonal test map (the same as `samples/sprite`).
- `resources/sprites/sunny_idle_down.png`: Sunny.
- `resources/sprites/glow.png`: a warm radial glow, 128 x 128. Its alpha falls off from the centre, and its transparent pixels keep the warm colour.
- `resources/sprites/shadow.png`: a soft bluish ellipse, 128 x 64. Its transparent pixels are white (see above).
