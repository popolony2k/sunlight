# gamepad sample

Builds on [collision](/samples/collision/docs/README.md) by dropping the collision rule and giving
each character its own independent controller instead: Sunny is driven by the gamepad's **left**
stick/DPad, and the monkey is driven by the **right** stick/face buttons — demonstrating that a
single `TileMapRenderer` can dispatch both sticks of one connected gamepad to two different sprites
via `SetUserGamePadEventHandler`.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `gamepad_test` executable under `build/samples/gamepad/`, along with the
raylib/tmx/LibXml2/sunlight shared libraries copied next to it (required at runtime).

## Running

`gamepad_test` resolves its map/sprite resources relative to a base path passed as `argv[1]` —
point it at this sample's own directory:

```shell
./build/samples/gamepad/gamepad_test samples/gamepad/
```

## Controls

| Input | Action |
| --- | --- |
| `W` / `A` / `S` / `D` | Move Sunny (keyboard fallback) |
| Gamepad left stick / DPad | Move Sunny |
| `Up` / `Down` / `Left` / `Right` arrows | Move the monkey (keyboard fallback) |
| Gamepad right stick / face buttons (Y/A/X/B on Xbox, Triangle/Cross/Square/Circle on PS) | Move the monkey |
| `Page Up` | Zoom out |
| `Page Down` | Zoom in |
| `Home` | Reset zoom to the default level |

Keyboard controls work with no gamepad connected at all, so the sample can be exercised without
hardware — the gamepad bindings are additive, not a replacement.

## What it shows

- `World::Run()` ([world.cpp](/samples/gamepad/world.cpp)) calls `TileMapRenderer::AddGamePad(0)` to
  tell the renderer to start dispatching input from gamepad id `0` — a gamepad's events are ignored
  by `HandleUserInput()` until its id has been registered this way.
- `SetUserGamePadEventHandler(GAMEPAD_BUTTON_LEFT_FACE_UP/DOWN/LEFT/RIGHT, ...)` wires Sunny's
  movement to the left stick/DPad; `SetUserGamePadEventHandler(GAMEPAD_BUTTON_RIGHT_FACE_UP/DOWN/LEFT/RIGHT, ...)`
  wires the monkey's movement to the right stick/face buttons. Both the physical DPad/face buttons
  and tilting the corresponding analog stick fire the same handler — `TileMapRenderer` maps stick
  tilt onto the same virtual button events internally (see `HandleUserInput()`), so no branching on
  input source is needed in sample code.
- Each handler also accepts the plain `ControllerType`/id parameters passed to every
  `SetUserKeyEventHandler`/`SetUserGamePadEventHandler` callback, but this sample ignores them since
  it doesn't need to distinguish which controller triggered the move.
- Sunny and the monkey are loaded as two independent `Sprite`s, same as in the `collision` sample,
  but with no `CollisionManager` rule between them — they can freely overlap here.

## Resources

- `resources/map/` — the Tiled `.tmx` map and its tilesets, shared with the other samples.
- `resources/sprites/sunny_idle_down.png` — the sprite sheet used for Sunny.
- `resources/map/images/monke_variants.png` — the tileset used for the monkey (tile 0 of its 4x4 grid).
