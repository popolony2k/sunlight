# input sample

Shows `sunlight`'s input configuration on a drawing surface with no map: named actions that can be rebound while the game runs and saved to a file (`ActionMap`), gamepads joining and leaving as players (`PlayerSlotManager`), connect/disconnect events, and a stick dead zone changed live.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `input_test` executable under `build/samples/input/`, along with the shared libraries copied next to it.

## Running

The sample loads no resources, so it needs no base path (one given as `argv[1]`, like the other samples take, is ignored):

```shell
./build/samples/input/input_test
```

Gamepads are optional: with none, only the keyboard player and its bindings are shown.

## Controls

| Input | Action |
| --- | --- |
| Arrow keys (rebindable) | Move the keyboard player `K` |
| `F1` / `F2` / `F3` / `F4` | Rebind Left / Right / Up / Down: the next key pressed becomes its binding |
| `F5` / `F9` | Save / load every binding |
| `A` on a gamepad (`Cross` on PlayStation) | Join as the next free player, `P1` or `P2` |
| `B` on a gamepad (`Circle` on PlayStation) | Leave that player slot |
| Left stick | Move that pad's player |
| `[` / `]` | Lower / raise the left stick's dead zone (steps of 0.05, from 0 to 0.9) |
| `Esc` | Quit |

## What to look for

- **Actions:** the keyboard player moves through the actions "Left", "Right", "Up" and "Down", polled every frame with `ActionMap::IsActionActive`, never through the raw keys. Rebind one (e.g. `F1`, then `A`) and the new key moves `K` at once. A key another action already has is refused, and the status line says which action holds it. The sample's own keys (`F1`-`F5`, `F9`, `[`, `]`, `Esc`) can't be taken.
- **Save and load:** `F5` writes every binding to `sunlight_input_bindings.txt` in the system's temporary folder (the console prints the full path at start-up), and `F9` reads it back, undoing any rebinding made since.
- **Player slots:** pressing `A` on a pad gives it the first free slot; its player appears. Unplug that pad and its player turns grey, and the panel shows the slot as "unplugged": the slot keeps its pad. Plug the same pad back and it is the same player again, with no new claim. `B` frees the slot for another pad.
- **Connect/disconnect events:** the panel lists the last few, with each pad's name. Pads already plugged in when the sample starts are reported in its first frame.
- **Dead zone:** the panel shows each pad's raw left-stick values. A stick moves its player only past the pad's dead zone, so with a high dead zone a small tilt does nothing, and at 0 the slightest touch moves it.

## In the code

- `World::BindDefaults()` ([world.cpp](/samples/input/world.cpp)) binds the keyboard actions to the arrow keys, and four pad actions to the two directions of the left stick's axes (`BindGamepadAxis`). One action map serves every pad: the pad to read is passed to `IsActionActive` when the action is polled.
- `World::HandleRebinding()` reads the next key with `IInputHandler::GetKeyPressed` and calls `ActionMap::BindKey`; `World::HandleSaveLoad()` calls `ActionMap::Save`/`Load`.
- `World::HandleSlots()` claims and releases slots with `PlayerSlotManager::ClaimSlot`/`ReleaseSlot`, for every pad in `IControllerManager::GetConnectedGamepads()`.
- `World::Run()` registers the connect/disconnect handler with `IControllerManager::SetGamepadConnectionHandler`, and sets each new pad's dead zone.
- `World::HandleDeadzone()` calls `IControllerManager::SetAxisDeadzone` on both left-stick axes of every pad; `World::MovePlayers()` passes the pad's `GetAxisDeadzone` as the threshold to `IsActionActive`.
- Everything is polled and drawn from `World::OnUpdate()`, once per frame. The panel and players are drawn with `IDrawSurface::DrawText` and `DrawFilledRectangle`.

`DefaultControllerBindings` (the arrow-key camera controls) is shown by the `objects`, `text`, `opacity` and `shapes` samples instead: handlers can't be removed once installed, so it can't be switched off again here.
