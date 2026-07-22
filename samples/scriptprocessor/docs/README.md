# scriptprocessor sample

Demonstrates `SunLight::Scripting::ScriptProcessor` by driving a small "stage intro" cutscene:
a queued script loads a stage, slides Sunny and a monkey onto screen, plays/pauses/resumes a
background tone through `SoundManager`, then settles into an idle loop — while Sunny walks a
continuous circle and the monkey patrols a rectangle in the background, both independent of the
script's own timing.

## Building

From the project root, enable samples and build:

```shell
cmake -B build -S . -DBUILD_LIBRARY_SAMPLES=ON
cmake --build build -j 4
```

This produces the `scriptprocessor_test` executable under `build/samples/scriptprocessor/`, along
with the raylib/tmx/LibXml2/sunlight shared libraries copied next to it (required at runtime).

## Running

`scriptprocessor_test` resolves its map/sprite/sound resources relative to a base path passed as
`argv[1]` — point it at this sample's own directory:

```shell
./build/samples/scriptprocessor/scriptprocessor_test samples/scriptprocessor/
```

## Controls

| Keys | Action |
| --- | --- |
| `Page Up` | Zoom out |
| `Page Down` | Zoom in |
| `Home` | Reset zoom to the default level |
| `M` | Stop the background tone directly (see "Why `M` stops the music" below) |

This sample is mostly a non-interactive cutscene — there's no player-controlled sprite, the script
drives everything.

## What it shows

`World::BuildScript()` ([world.cpp](/samples/scriptprocessor/world.cpp)) queues every
`ScriptProcessor` command except the `*_DIRECT_CMD` family (see below), in this order:

1. **`LOAD_STAGE_CMD(1)`** — `World::LoadStage()` resets both sprites to hidden, off-screen
   positions. Each id passed to `LOAD_STAGE_CMD`/`MOVE_SPRITES_TO_SCREEN_CMD`/`PLAY_SONG_CMD` is a
   hook `World::OnCommand()` switches on — "each id means a sprite/stage/song action", the same
   pattern a full game's script layer would use to drive many different objects from one processor.
2. **`WAIT_CMD(1000)`** — a one-second pause before anything happens.
3. **`PLAY_SONG_CMD(1)`** — starts the stage theme via `SoundManager`.
4. **`MOVE_SPRITES_TO_SCREEN_CMD(1)`** then **`MOVE_SPRITES_TO_SCREEN_CMD(2)`** (with a
   `WAIT_CMD(400)` between them) — trigger Sunny's and the monkey's slide-in animations. Both
   start off-screen far enough that `Viewport::GetClippedRect()` naturally excludes them from
   drawing until they slide into the visible range.
5. **`WAIT_SPRITES_QUEUE_EMPTY`** — pauses the script until both slide-in animations finish.
   `World::OnUpdate()` (an `ITileMapListener` callback, called once per frame) advances any
   in-flight slide and calls `ScriptProcessor::ResetWaitSpritesQueueEmptyCmd()` once none are left
   — that's the "sprites queue" the command name refers to; `ScriptProcessor` itself has no notion
   of sprite animation, it just exposes the wait/reset pair for the game code to drive.
6. **`PAUSE_SONG_CMD(1)`**, **`WAIT_CMD(500)`**, **`RESUME_SONG_CMD(1)`** — pauses and resumes the
   tone. `RayLibSound::Resume()` is a thin wrapper over raylib's `ResumeSound()`, which continues
   from wherever the stream currently is rather than restarting — the stage theme is deliberately
   6 seconds long with only a short attack/release envelope (not a fade across its whole length)
   so there's plenty of audible tone left when pause/resume actually fire.
7. **`LABEL_CMD`**, **`LOOP_CMD`/`END_LOOP_CMD`**, **`GOTO_LABEL_CMD`** — an idle "heartbeat"
   segment: `WAIT_CMD` a few times inside a bounded `LOOP_CMD`/`END_LOOP_CMD`, then
   `GOTO_LABEL_CMD` repeats the whole segment forever. `ScriptProcessor` has no conditional branch
   other than `LOOP_CMD`'s own counter, so any backward jump like this is inherently an infinite
   "attract mode" loop, not a bounded one — Sunny's circle and the monkey's rectangle patrol run on
   their own per-frame timing throughout, independent of this loop's pace.

### Why `M` stops the music

Because the idle segment loops forever via `GOTO_LABEL_CMD`, the script never reaches a scripted
`STOP_SONG_CMD` — there's nothing after the infinite loop for it to reach. `M` is wired directly to
`World::OnCommand(STOP_SONG_CMD, ...)`, the exact same dispatcher a script would use, just invoked
from a keyboard handler instead of a queued command.

### The `*_DIRECT_CMD` family

`PLAY_SONG_DIRECT_CMD`/`PAUSE_SONG_DIRECT_CMD`/`RESUME_SONG_DIRECT_CMD`/`STOP_SONG_DIRECT_CMD` are
intentionally not queued in this sample's script — `ScriptProcessor::Run()` has a no-op case for
all four ("Not handled by scripts"), so they're meant to be invoked directly rather than added to
a script's command queue.

## Resources

- `resources/map/` — the Tiled `.tmx` map and its tilesets, shared with the other samples.
- `resources/sprites/sunny_idle_down.png` — the sprite sheet used for Sunny.
- `resources/map/images/monke_variants.png` — the tileset used for the monkey (tile 0 of its 4x4 grid).
- `resources/sounds/stage_theme.wav` — a synthesized tone (no external asset/licensing needed),
  generated as a 6-second 440Hz sine wave with a short attack/release envelope.
