# isometric_haptics sample

The [isometric sample](/samples/isometric/docs/README.md) with gamepad support and rumble: Sunny can be walked with a gamepad, and the gamepad vibrates when she walks into an obstacle. The harder she was running, the stronger the bump.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `isometric_haptics_test` executable under `build/samples/isometric_haptics/`, along with the shared libraries copied next to it.

## Running

`isometric_haptics_test` resolves its resources relative to a base path passed as `argv[1]`:

```shell
./build/samples/isometric_haptics/isometric_haptics_test samples/isometric_haptics/
```

Connect a gamepad that can vibrate (gamepad 0) to feel the rumble. Without one, the sample runs exactly like the isometric one.

## Controls

| Keys | Gamepad | Action |
| --- | --- | --- |
| `W` / `Up Arrow` | D-pad up / left stick up | One cell up-right (row - 1) |
| `S` / `Down Arrow` | D-pad down / left stick down | One cell down-left (row + 1) |
| `A` / `Left Arrow` | D-pad left / left stick left | One cell up-left (column - 1) |
| `D` / `Right Arrow` | D-pad right / left stick right | One cell down-right (column + 1) |
| `=` / `-` | | Zoom in / out |
| `Home` | | Reset the zoom |
| `Esc` | | Quit |

## What to look for

- Walking into an obstacle cell rumbles the gamepad for 0.2 s. A bump straight after turning is weak (strength 0.2). Each step already taken in the same direction makes it stronger, up to full strength after six steps.
- Holding a direction against a wall bumps once, not continuously: only the first blocked attempt of a hold rumbles.
- Walking off the map's edge doesn't rumble. Only an obstacle tile counts as a wall.

## In the code

- `World::LoadSprites()` ([world.cpp](/samples/isometric_haptics/world.cpp)) creates the haptics with `GamepadHapticsFactory::CreateGamepadHaptics(GetInputHandler())`. `World::Run()` registers gamepad 0 with `AddGamePad`, and gamepad handlers next to the keyboard ones.
- `World::TriggerWallBump()` checks `IGamepadHaptics::IsSupported` and calls `SetVibration` with the strength for the run-up.
- `World::OnUpdate()` calls `IGamepadHaptics::Update()` once per frame. The rumble backend (SDL2) only stops a timed vibration from inside that call.

## Assets

The same as the [isometric sample](/samples/isometric/docs/README.md).
